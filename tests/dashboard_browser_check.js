// Render actual embedded assets with mocked fetches; never connects to a robot.
const [socketUrl, inputPath] = Deno.args;
const input = JSON.parse(await Deno.readTextFile(inputPath));
const socket = new WebSocket(socketUrl);
await new Promise((resolve, reject) => { socket.onopen = resolve; socket.onerror = reject; });
let sequence = 0;
const pending = new Map();
socket.onmessage = event => {
  const message = JSON.parse(event.data);
  if (!message.id) return;
  const request = pending.get(message.id);
  pending.delete(message.id);
  message.error ? request.reject(message.error) : request.resolve(message.result);
};
const call = (method, params = {}) => new Promise((resolve, reject) => {
  const id = ++sequence;
  pending.set(id, { resolve, reject });
  socket.send(JSON.stringify({ id, method, params }));
});
const evaluate = async expression => {
  const result = await call('Runtime.evaluate', { expression, returnByValue:true, awaitPromise:true });
  if (result.exceptionDetails) throw new Error(JSON.stringify(result.exceptionDetails));
  return result.result.value;
};
const asset = (source, delimiter) => source.match(new RegExp('R"' + delimiter + '\\(([\\s\\S]*?)\\)' + delimiter + '";'))[1];
const states = [
  { name:'normal', mode:'MANUAL', hazard:false, override:false },
  { name:'flame-blocked', mode:'MANUAL', hazard:true, override:false },
  { name:'gas-blocked', mode:'MANUAL', hazard:true, override:false, gas:true },
  { name:'flame-override', mode:'MANUAL', hazard:true, override:true },
  { name:'gas-override', mode:'MANUAL', hazard:true, override:true, gas:true },
  { name:'auto', mode:'AUTO', hazard:false, override:false }
];
const viewportSizes = [360,375,390,412,430].flatMap(width => [640,800].map(height => ({width,height})))
  .concat([{width:1180,height:800}]);
const results = {};
await call('Page.enable');
for (const variant of ['frozen','baseline','current']) {
  const source = input[variant];
  // Pending mocked responses prevent polling from overwriting explicit fixtures.
  // Original initialization and event bindings still execute unchanged.
  const mock = `window.__requests=[];window.__errors=[];
    window.addEventListener('error',e=>window.__errors.push(e.message));
    window.fetch=(url,options)=>{window.__requests.push({url:String(url),method:options?.method||'GET',body:options?.body?.toString()||''});return new Promise(()=>{});};`;
  const html = asset(source,'html').replace('<link rel="stylesheet" href="/dashboard.css">', '<style>'+asset(source,'css')+'</style>')
    .replace(/<script[^>]*src="\/dashboard.js"[^>]*><\/script>/, '<script>'+mock+'</script><script>'+asset(source,'js')+'</script>');
  await call('Page.navigate', {url:'data:text/html,'+encodeURIComponent('<html data-variant="'+variant+'"></html>')+'#control'});
  const tree = await call('Page.getFrameTree');
  await call('Page.setDocumentContent', {frameId:tree.frameTree.frame.id, html});
  results[variant] = {controls:[],pages:[],sonar:[],alarms:[],handlerRequests:[]};
  for (const {width,height} of viewportSizes) {
    await call('Emulation.setDeviceMetricsOverride', {width,height,deviceScaleFactor:1,mobile:width<680});
    for (const state of states) {
      const fixture = {
        mode:state.mode,turnMode:'PIVOT',motorState:'STOP',navigationDecision:'Manual ready',
        safetyState:state.hazard?'CRITICAL':'SAFE',alarmReason:state.hazard?(state.gas?'GAS_HIGH':'FLAME_CRITICAL'):'NONE',
        alarmSoundState:'DISABLED',buzzerState:'DISABLED',ultrasonicState:'CLEAR_NO_ECHO',distanceValid:false,distance:null,
        temperature:26,humidity:45,telemetry_age_ms:0,speed_percent:60,commandSequence:0,
        flame_sample_valid:true,flameDetected:state.hazard&&!state.gas,gasStatus:state.gas?'HIGH_RELATIVE':'LOW_RELATIVE',
        scanOrientation:'FRONT',servoAngle:90,manualMotionInhibitReason:'NONE',
        imu:{},radio:{},display:{},autoNavigation:{},
        ...(true ? {flameCritical:state.hazard&&!state.gas,gasCritical:Boolean(state.gas),manualFlameGasOverride:state.override,controlSessionAlive:true}
          : {local_hazard_active:state.hazard,local_hazard_type:state.gas?'GAS':state.hazard?'FLAME':'NONE',manual_override_active:state.override})
      };
      const observation = await evaluate(`(()=>{
        window.__fixture=${JSON.stringify(fixture)};mode=window.__fixture.mode;
        if(typeof controlToken!=='undefined')controlToken='mock-session';
        if(typeof controlSessionToken!=='undefined')controlSessionToken='mock-session';
        renderTelemetry(window.__fixture);activateSection('control',false);window.scrollTo(0,0);
        const rect=e=>{const r=e.getBoundingClientRect();return {x:r.x,y:r.y,width:r.width,height:r.height,bottom:r.bottom};};
        const row=document.querySelector('.control-alarm-bar'),rowStyle=getComputedStyle(row);
        const buttons=Array.from(row.children).map(e=>({id:e.id,...rect(e)}));
        const holds=Array.from(document.querySelectorAll('.hold')).map(e=>({disabled:e.disabled,aria:e.getAttribute('aria-disabled'),opacity:getComputedStyle(e).opacity}));
        const banner=document.querySelector('#alarmIndicator'),override=document.querySelector('#overrideBtn'),alert=document.querySelector('#overrideAlert');
        const sharedSelectors=['.mode-bar','.turn-bar','.speed-control','.control-note','.control-alarm-bar','.dpad','.sensor-head-control','.sensor-head-buttons','.sensor-head-btn'];
        const sharedStyles=sharedSelectors.map(selector=>{
          const e=document.querySelector('#control '+selector),s=getComputedStyle(e);
          return {selector,width:rect(e).width,display:s.display,columns:s.gridTemplateColumns,gap:s.gap,
            margin:s.margin,padding:s.padding,fontSize:s.fontSize,background:s.backgroundColor,border:s.borderRadius,minHeight:s.minHeight};
        });
        const order=['.mode-bar','.turn-bar','.speed-control','#controlAvailability','.control-alarm-bar','#dpad','.sensor-head-control','.control-readouts','.radar-column'].map(selector=>({selector,...rect(document.querySelector('#control '+selector))}));
        const problems=[];const nav=document.querySelector('.section-nav');
        const operatorControls=Array.from(document.querySelectorAll('#control button,#motorSpeed')).filter(e=>!e.hidden);
        if(${width}<680) {
          for(const e of operatorControls){
            e.scrollIntoView({block:'center',inline:'nearest',behavior:'instant'});
            const r=e.getBoundingClientRect(),n=nav.getBoundingClientRect(),h=document.querySelector('.app-header').getBoundingClientRect();
            if(r.left<0||r.right>innerWidth+1||r.top<h.bottom-1||r.bottom>n.top-4)problems.push('clearance:'+e.id);
            for(const [x,y] of [[r.left+5,r.top+5],[r.right-5,r.bottom-5],[r.left+r.width/2,r.top+r.height/2]])
              if(!e.contains(document.elementFromPoint(x,y)))problems.push('hit-test:'+e.id);
          }
          const dpad=document.querySelector('#dpad');dpad.scrollIntoView({block:'center',behavior:'instant'});
          if(dpad.getBoundingClientRect().top<document.querySelector('.app-header').getBoundingClientRect().bottom||dpad.getBoundingClientRect().bottom>nav.getBoundingClientRect().top-4)problems.push('whole D-pad');
          window.scrollTo(0,document.documentElement.scrollHeight);
          if(document.querySelector('#control .radar-column article:last-child').getBoundingClientRect().bottom>nav.getBoundingClientRect().top)problems.push('bottom clearance');
        }
        if(document.documentElement.scrollWidth>innerWidth||document.body.scrollWidth>innerWidth)problems.push('overflow');
        return {width:innerWidth,height:innerHeight,state:${JSON.stringify(state.name)},gridColumns:rowStyle.gridTemplateColumns,
          gridGap:rowStyle.gap,buttons,holds,stopDisabled:document.querySelector('#stopBtn').disabled,status:document.querySelector('#controlAvailability').textContent,
          overrideText:override.textContent,overrideHidden:override.hidden,overrideDisabled:override.disabled,alertText:alert.textContent,alertHidden:alert.hidden,
          bannerText:banner.querySelector('.alarm-label').textContent,bannerDetail:banner.querySelector('.alarm-detail').textContent,bannerClass:banner.className,
          bannerColor:getComputedStyle(banner).backgroundColor,order,sharedStyles,problems,errors:window.__errors};
      })()`);
      results[variant].controls.push(observation);
      if (variant==='current' && width===390 && height===800 && ['normal','flame-blocked','flame-override'].includes(state.name) && input.screenshotDir) {
        await evaluate(`document.querySelector('#dpad').scrollIntoView({block:'center',behavior:'instant'});`);
        const screenshot = await call('Page.captureScreenshot', {format:'png'});
        await Deno.writeFile(input.screenshotDir+'/'+state.name+'.png', Uint8Array.from(atob(screenshot.data),c=>c.charCodeAt(0)));
      }
    }
    // Non-target pages compare against Stage 4 baseline, with the one approved title change excluded.
    for (const page of ['overview','sensors','communication','system']) {
      const pageResult = await evaluate(`(()=>{
        const d={...window.__fixture,mode:'MANUAL',local_hazard_active:false,manual_override_active:false,manualFlameGasOverride:false,
          flameDetected:false,flameCritical:false,gasCritical:false,gasStatus:'LOW_RELATIVE',alarmReason:'NONE',safetyState:'SAFE'};
        mode='MANUAL';renderTelemetry(d);activateSection('${page}',false);window.scrollTo(0,0);
        const section=document.getElementById('${page}');
        const rounded=n=>Math.round(n*100)/100;
        const layout=Array.from(section.querySelectorAll('.panel,.metric-card,button,input,select')).map(e=>{
          const r=e.getBoundingClientRect(),s=getComputedStyle(e);
          return [e.id,e.className,rounded(r.x),rounded(r.y),rounded(r.width),rounded(r.height),s.display,s.gridTemplateColumns,s.gap,s.padding,s.fontSize,s.backgroundColor];
        });
        const nav=document.querySelector('.section-nav'),n=nav.getBoundingClientRect(),ns=getComputedStyle(nav);
        const panel=document.querySelector('#alarmSound').closest('article');
        return {width:innerWidth,height:innerHeight,page:'${page}',layout,overflow:document.documentElement.scrollWidth>innerWidth||document.body.scrollWidth>innerWidth,
          nav:[nav.outerHTML,rounded(n.x),rounded(n.y),rounded(n.width),rounded(n.height),ns.position,ns.bottom],
          alarmHeading:panel.querySelector('h3').textContent,alarmButtons:panel.querySelectorAll('.alarm-btn').length};
      })()`);
      results[variant].pages.push(pageResult);
    }
  }
  results[variant].sonar = await evaluate(`(()=>{activateSection('control',false);return [
    ['DISTANCE_VALID',true,42],['CLEAR_NO_ECHO',false,null],['SENSOR_FAULT',false,null],['STALE',false,null]
  ].map(([ultrasonicState,distanceValid,distance])=>{renderTelemetry({...window.__fixture,ultrasonicState,distanceValid,distance});return [document.querySelector('#dist').textContent,document.querySelector('[data-bind="distanceValidity"]').textContent];});})()`);
  await evaluate(`window.__requests=[];document.querySelector('#enableAlarmBtn').click();document.querySelector('#testBeepBtn').click();document.querySelector('#muteAlarmBtn').click();`);
  results[variant].alarms = await evaluate('window.__requests.filter(r=>r.url.startsWith("/alarm"))');
  if (variant!=='frozen') {
    await evaluate(`window.__requests=[];mode='MANUAL';controlSessionToken='mock-session';latestData={...window.__fixture,mode:'MANUAL',flameCritical:true,controlSessionAlive:true,manualFlameGasOverride:false};renderTelemetry(latestData);document.querySelector('#overrideBtn').click();`);
    results[variant].handlerRequests = await evaluate('window.__requests');
  }
}
console.log(JSON.stringify(results));
socket.close();
