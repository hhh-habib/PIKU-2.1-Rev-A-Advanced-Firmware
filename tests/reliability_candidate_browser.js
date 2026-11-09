// Actual embedded JS/CSS/HTML in Chromium. All HTTP is doubled; no robot access.
const [socketUrl, inputPath] = Deno.args;
const input = JSON.parse(await Deno.readTextFile(inputPath));
const ws = new WebSocket(socketUrl);
await new Promise((ok, bad) => { ws.onopen = ok; ws.onerror = bad; });
let id = 0;
let pageLoaded = null;
const pending = new Map();
ws.onmessage = e => { const r = JSON.parse(e.data); if (r.method === 'Page.loadEventFired' && pageLoaded) { pageLoaded(); pageLoaded=null; } if (!r.id) return; const p = pending.get(r.id); if(!p)return; pending.delete(r.id); r.error ? p.reject(r.error) : p.resolve(r.result); };
const call = (method, params = {}) => new Promise((resolve, reject) => { const n = ++id; pending.set(n, {resolve, reject}); ws.send(JSON.stringify({id:n, method, params})); });
const evaluate = async expression => { const r = await call('Runtime.evaluate', {expression, returnByValue:true, awaitPromise:true}); if (r.exceptionDetails) throw Error(JSON.stringify(r.exceptionDetails)); return r.result.value; };
const asset = (source, tag) => source.match(new RegExp('R"'+tag+'\\(([\\s\\S]*?)\\)'+tag+'";'))[1];
const mock = `
window.__networkFetch=window.fetch.bind(window);
window.__requests=[];window.__errors=[];window.__clock=0;window.__ignoreAbort=false;
window.addEventListener('error',e=>window.__errors.push(e.message));
let timerId=0;const timers=new Map();
window.setTimeout=(fn,ms=0)=>{const id=++timerId;timers.set(id,{fn,at:__clock+ms});return id;};
window.clearTimeout=id=>timers.delete(id);
window.setInterval=(fn,ms)=>{const id=++timerId;timers.set(id,{fn,at:__clock+ms,repeat:ms});return id;};
window.clearInterval=window.clearTimeout;
window.__settle=async()=>{for(let i=0;i<30;i++)await Promise.resolve();};
window.__advance=async ms=>{const target=__clock+ms;for(let limit=0;limit<20000;limit++){
 const entry=[...timers].filter(([,t])=>t.at<=target).sort((a,b)=>a[1].at-b[1].at)[0];if(!entry){__clock=target;return;}
 const [id,t]=entry;__clock=t.at;timers.delete(id);if(t.repeat)timers.set(id,{...t,at:t.at+t.repeat});t.fn();await __settle();
}throw Error('timer execution did not converge');};
window.fetch=(url,options={})=>new Promise((resolve,reject)=>{
 const r={url:String(url),options,resolve,reject};__requests.push(r);
 if(options.signal&&!__ignoreAbort)options.signal.addEventListener('abort',()=>reject(Error('transport aborted')),{once:true});
});
window.__fixture={mode:'MANUAL',turnMode:'PIVOT',motorState:'STOP',navigationDecision:'MANUAL_CONTROL',
 safetyState:'SAFE',alarmReason:'NONE',alarmSoundState:'DISABLED',buzzerState:'NOT_INSTALLED',
 ultrasonicState:'DISTANCE_VALID',distanceValid:true,distance:80,temperature:26,humidity:45,
 telemetry_age_ms:0,speed_percent:60,commandSequence:0,flame_sample_valid:true,gasStatus:'LOW_RELATIVE',
 scanOrientation:'FRONT',servoAngle:90,manualMotionInhibitReason:'NONE',controlSessionAlive:true,
 imu:{},radio:{},display:{},autoNavigation:{},uptimeMs:10000};
let revision=0;
window.__ack=(r,body=__fixture)=>r.resolve({ok:true,status:200,headers:new Headers({'X-PIKU-Mode-Fence':'123.'+(++revision),...r.controlHeaders}),json:()=>Promise.resolve(body)});
window.__stops=()=>__requests.filter(r=>r.url.startsWith('/cmd?move=STOP'));
window.__modes=()=>__requests.filter(r=>r.url.startsWith('/mode?'));
window.__check=(ok,message)=>{if(!ok)throw Error(message);};
`;
await call('Page.enable');
const reset = async (source = input.assets) => {
  const loaded = new Promise(resolve => { pageLoaded=resolve; });
  await call('Page.navigate', {url:input.nativeEndpoint ? input.nativeEndpoint+'/fixture' : 'about:blank'});
  await loaded;
  const tree = await call('Page.getFrameTree');
  const html = asset(source,'html').replace('<link rel="stylesheet" href="/dashboard.css">','<style>'+asset(source,'css')+'</style>')
    .replace(/<script[^>]*src="\/dashboard.js"[^>]*><\/script>/,'<script>'+mock+'</script><script>'+asset(source,'js')+'</script>');
  await call('Page.setDocumentContent', {frameId:tree.frameTree.frame.id, html});
  await evaluate(`(async()=>{mode='MANUAL';renderTelemetry(__fixture);for(const request of __requests.filter(r=>r.url==='/control-session'))__ack(request,{token:'fixture-session'});await __settle();clearMoveRefresh();if(dataAbortController)dataAbortController.abort();if(pingAbortController)pingAbortController.abort();await __settle();__requests=[];})()`);
};
const results = {behavior:[],layout:[]};
const scenario = async (name, body) => { await reset(); await evaluate(`(async()=>{${body};__check(__errors.length===0,'browser error: '+__errors);return true;})()`); results.behavior.push(name); };
// HTTP response latency is independent of command admission. The motor/deadman
// here are a chronological model; separate native tests execute firmware bodies.
const manualServer = `
window.__wire={mode:'AUTO',motion:'STOP',sequence:0,last:0,accepted:0,token:'fixture-session',sessionAlive:true};
window.__responseDelay=650;window.__dropDirections=false;window.__dropAll=false;
const originalFetch=fetch;
fetch=(url,options)=>{const promise=originalFetch(url,options),request=__requests.at(-1),u=new URL(url,'http://fixture');
 const move=u.searchParams.get('move'),direction=move&&!['STOP','RELEASE'].includes(move);
 if(__dropAll||(__dropDirections&&direction))return promise;
 let rejected=false;
 if(move){const sequence=Number(u.searchParams.get('seq'));if(move==='STOP'||sequence>__wire.sequence){
  __wire.sequence=Math.max(__wire.sequence,sequence);__wire.motion=direction&&__wire.mode==='MANUAL'?move:'STOP';__wire.last=__clock;if(direction)__wire.accepted++;
 }else rejected=true;}
 if(u.pathname==='/mode')__wire.mode=u.searchParams.get('set');
 if(u.pathname==='/control-session'){++__wire.sequence;__wire.motion='STOP';if(!__wire.sessionAlive)__wire.token='renewed-session';__wire.sessionAlive=true;}
 request.controlHeaders={'X-PIKU-Command-Sequence':String(__wire.sequence),'X-PIKU-Control-State':__wire.mode!=='MANUAL'?'AUTO':!__wire.sessionAlive?'MANUAL_SESSION_EXPIRED':u.searchParams.get('token')===__wire.token?'MANUAL_AUTHORIZED':'MANUAL_OTHER_SESSION'};
 const body=u.pathname==='/control-session'?{token:__wire.token,commandSequence:__wire.sequence}:
  u.pathname==='/ping'?{ok:true}:{...__fixture,mode:__wire.mode,motorState:__wire.motion,commandSequence:__wire.sequence,controlSessionAlive:__wire.sessionAlive};
 setTimeout(()=>rejected?request.resolve({ok:false,status:409}):__ack(request,body),direction?__responseDelay:5);
 return promise;
};
window.__step=async duration=>{for(let spent=0;spent<duration;spent+=25){await __advance(Math.min(25,duration-spent));
 if(__wire.motion!=='STOP'&&__clock-__wire.last>=750)__wire.motion='STOP';}};
mode='AUTO';renderTelemetry({...__fixture,mode:'AUTO'});setMode('MANUAL');await __step(50);
__check(mode==='MANUAL'&&!controlRequestUncertain&&__modes().length===1,'initial Manual selection');
`;
await scenario('timeout_confirmed_stop_next_fresh_press', manualServer+`
 __responseDelay=5000;startMove('FORWARD');await __step(2200);
 __check(__wire.motion==='STOP'&&!activePress&&!pendingMove,'uncertain command did not stop');
 __check(!controlRequestUncertain&&[...document.querySelectorAll('.hold')].every(b=>!b.disabled),'confirmed STOP and valid authority still require MANUAL');
 __responseDelay=20;startMove('LEFT');await __step(1000);__check(__wire.motion==='LEFT'&&activePress&&__modes().length===1,'fresh direction required mode reselection');stopMove();await __step(50);
 startMove('FORWARD');await __step(1000);stopMove();await __step(50);__check(__wire.motion==='STOP'&&!controlRequestUncertain&&__modes().length===1,'repeat after recovery required Manual');
`);
await scenario('session_release_sequence_before_first_direction', `controlSessionToken=null;const releasedSequence=commandSequence+1;ensureControlSession();__ack(__requests.find(r=>r.url==='/control-session'),{token:'fresh-session',commandSequence:releasedSequence});await __settle();startMove('FORWARD');const command=__requests.find(r=>r.url.includes('move=FORWARD'));__check(Number(new URL(command.url,'http://fixture').searchParams.get('seq'))>releasedSequence,'session RELEASE made first direction stale');`);
await scenario('first_press_waits_own_session_release', `controlSessionToken=null;const sequence=commandSequence+1;ensureControlSession();const session=__requests.find(r=>r.url==='/control-session');startMove('FORWARD');__check(__requests.every(r=>!r.url.includes('move=FORWARD')),'direction overtook session RELEASE');__ack(session,{token:'fresh-session',commandSequence:sequence});await __settle();__check(activePress&&__requests.some(r=>r.url.includes('move=FORWARD')&&Number(new URL(r.url,'http://fixture').searchParams.get('seq'))>sequence),'fresh held press did not proceed after session response');stopMove();`);
await scenario('released_press_not_replayed_after_session', `controlSessionToken=null;ensureControlSession();const session=__requests.find(r=>r.url==='/control-session');startMove('FORWARD');stopMove();__ack(session,{token:'fresh-session',commandSequence:10});await __settle();__check(!activePress&&!pendingMove&&__requests.every(r=>!r.url.includes('move=FORWARD')),'released intent replayed after session completion');`);
await scenario('failed_session_cannot_replay_held_press', `controlSessionToken=null;ensureControlSession();const session=__requests.find(r=>r.url==='/control-session');startMove('FORWARD');session.reject(Error('connection lost'));await __settle();__check(!activePress&&!pendingMove&&controlRequestUncertain&&__stops().length===1&&__requests.every(r=>!r.url.includes('move=FORWARD')),'failed session completion replayed old held intent');`);
await scenario('confirmed_stop_waits_own_pending_authority', `controlSessionToken=null;ensureControlSession();const session=__requests.find(r=>r.url==='/control-session');inhibitControl('Command not confirmed');sendCmd('STOP');const stop=__stops()[0];stop.controlHeaders={'X-PIKU-Control-State':'MANUAL_OTHER_SESSION','X-PIKU-Command-Sequence':'10'};__ack(stop);await __settle();__check(controlRequestUncertain,'STOP without own session response enabled controls');__ack(session,{token:'new-authority',commandSequence:11});await __settle();__check(!controlRequestUncertain&&!activePress&&!pendingMove,'confirmed STOP plus own authority did not restore readiness');startMove('LEFT');__check(__requests.some(r=>r.url.includes('move=LEFT')&&Number(new URL(r.url,'http://fixture').searchParams.get('seq'))>11),'fresh direction used stale session sequence');`);
await scenario('release_timeout_stop_proof_restores_manual', manualServer+`
 __responseDelay=20;startMove('FORWARD');await __step(500);const serve=fetch;
 fetch=(url,options)=>{if(url.includes('move=RELEASE')){__wire.motion='STOP';return originalFetch(url,options);}return serve(url,options);};
 stopMove();await __step(STOP_REQUEST_TIMEOUT_MS+50);__check(__wire.motion==='STOP'&&!controlRequestUncertain,'confirmed fallback STOP still requires Manual');
 startMove('BACKWARD');await __step(500);__check(__wire.motion==='BACKWARD'&&__modes().length===1,'fresh Backward unavailable after Release recovery');stopMove();
`);
await scenario('live_does_not_replace_stop_proof', manualServer+`
 startMove('FORWARD');const failed=__requests.at(-1);__dropAll=true;failed.reject(Error('transport failed'));await __settle();await __step(1000);
 renderTelemetry({...__fixture,mode:'MANUAL'});updateConnectionState('live');__check(controlRequestUncertain&&!activePress&&__wire.motion==='STOP','Live bypassed unknown STOP');
 __dropAll=false;explicitStop();await __step(50);__check(!controlRequestUncertain&&!activePress,'fresh confirmed STOP did not recover');
 startMove('LEFT');await __step(1000);__check(__wire.motion==='LEFT'&&__modes().length===1,'fresh press after confirmed STOP required Manual');stopMove();await __step(50);
`);
await scenario('another_client_authority_does_not_unlock', manualServer+`
 startMove('FORWARD');const failed=__requests.at(-1);__wire.token='different-owner';failed.reject(Error('transport failed'));await __settle();await __step(50);
 renderTelemetry({...__fixture,mode:'MANUAL',controlSessionAlive:true});updateConnectionState('live');
 __check(controlRequestUncertain&&!activePress&&__wire.motion==='STOP','other client authority enabled old owner');
 __check(__requests.filter(r=>r.url==='/control-session').length===1,'revoked client silently replaced live owner');
`);
await scenario('expired_session_after_confirmed_stop', manualServer+`
 startMove('FORWARD');const failed=__requests.at(-1);__wire.sessionAlive=false;failed.reject(Error('transport failed'));await __settle();await __step(50);
 __check(!controlRequestUncertain&&!activePress&&controlSessionToken==='renewed-session'&&__wire.motion==='STOP','confirmed STOP plus new authority failed recovery');
 __responseDelay=20;startMove('LEFT');await __step(500);__check(__wire.motion==='LEFT'&&__modes().length===1,'session sequence prevented next fresh direction');stopMove();await __step(50);
`);
await scenario('older_failed_stop_after_newer_stop_ack', manualServer+`
 const serve=fetch;let old;
 fetch=(url,options)=>{if(!old&&url.includes('move=STOP')){const promise=originalFetch(url,options);old=__requests.at(-1);return promise;}return serve(url,options);};
 sendCmd('STOP');sendCmd('STOP');await __step(50);old.reject(Error('old STOP response lost'));await __settle();
 __check(!controlRequestUncertain&&__wire.motion==='STOP','older failure overrode newer confirmed STOP');
`);
await scenario('late_recovery_cannot_cancel_new_mode', `
 inhibitControl('Command not confirmed');sendCmd('STOP');const old=__stops()[0];
 loadData();__ack(__requests.filter(r=>r.url.startsWith('/data')).at(-1));await __settle();setMode('AUTO');
 const newer=__stops()[1];old.controlHeaders=newer.controlHeaders={'X-PIKU-Control-State':'MANUAL_AUTHORIZED'};
 __ack(newer);await __settle();__check(__modes().length===0,'mode bypassed older STOP');
 __ack(old);await __settle();__check(controlRequestUncertain&&!!modeTransition&&__modes().length===1,'old proof restored Manual during new mode transaction');
 __ack(__modes()[0]);await __settle();__check(mode==='AUTO'&&!activePress&&!controlRequestUncertain,'old Manual proof changed newer AUTO');
`);
for(const delay of [650,20,900,1200]) await scenario('persistent_manual_'+delay+'ms', manualServer+`
 __responseDelay=${delay};__ignoreAbort=true;
 for(let cycle=0;cycle<3;cycle++){
  for(const direction of ['FORWARD','LEFT','FORWARD']){
   startMove(direction);const duration=direction==='LEFT'?1000:10000;
   for(let spent=0;spent<duration;spent+=25){await __step(25);
    __check(activePress&&!controlRequestUncertain&&[...document.querySelectorAll('.hold')].every(b=>!b.disabled),'healthy hold inhibited at '+spent+'ms with ${delay}ms responses');
    __check(__wire.motion===direction,'deadman expired despite valid held refresh');
    __check(moveRequestsInFlight.size<=3,'direction response overlap unbounded');
   }
   stopMove();await __step(50);__check(__wire.motion==='STOP'&&mode==='MANUAL'&&!controlRequestUncertain,'release lost readiness');
  }
 }
 __check(__modes().length===1,'redundant Manual selection');
`);
await scenario('manual_deadman_on_delivery_loss', manualServer+`
 __responseDelay=650;startMove('FORWARD');await __step(2000);__dropAll=true;const last=__wire.last;
 await __step(750);__check(__wire.motion==='STOP'&&__clock-last>=750,'expired Forward kept moving');
 await __step(3000);__check(!activePress&&!pendingMove&&controlRequestUncertain,'lost connection retained held intent');
 const count=__requests.filter(r=>r.url.includes('move=FORWARD')).length;__dropAll=false;renderTelemetry({...__fixture,mode:'MANUAL'});updateConnectionState('live');await __step(2000);
 __check(__requests.filter(r=>r.url.includes('move=FORWARD')).length===count&&__wire.motion==='STOP','reconnect replayed movement');
`);
await scenario('obsolete_direction_rejection_before_newer_ack', manualServer+`
 startMove('FORWARD');const old=__requests.at(-1);await __step(250);
 old.resolve({ok:false,status:409});await __settle();await __step(2000);
 __check(activePress&&!controlRequestUncertain&&__wire.motion==='FORWARD','obsolete response stopped fresh accepted refreshes');
 stopMove();await __step(50);__check(!controlRequestUncertain&&__wire.motion==='STOP','release after reordered response');
`);
await scenario('old_response_timeout_with_newer_ack', manualServer+`
 const fetchWithServer=fetch;let first=true;fetch=(url,options)=>{if(first&&url.includes('move=FORWARD')){first=false;return originalFetch(url,options);}return fetchWithServer(url,options);};
 startMove('FORWARD');await __step(3000);__check(activePress&&!controlRequestUncertain&&__wire.motion==='FORWARD','old timeout overrode newer accepted refresh');
 stopMove();await __step(50);__check(__wire.motion==='STOP'&&!controlRequestUncertain,'release after old timeout');
`);
await scenario('rejected_old_refresh_without_newer_ack', manualServer+`
 startMove('FORWARD');const old=__requests.at(-1);__dropAll=true;await __step(250);old.resolve({ok:false,status:409});await __settle();
 await __step(3000);__check(controlRequestUncertain&&!activePress&&!pendingMove&&__wire.motion==='STOP','rejection without newer proof failed open');
`);
await scenario('priority_stop_aborts_entire_refresh_window', manualServer+`
 __responseDelay=1200;__ignoreAbort=true;startMove('FORWARD');await __step(600);__check(moveRequestsInFlight.size===3,'refresh window not exercised');
 explicitStop();await __step(50);__check(__wire.motion==='STOP'&&!activePress&&moveRequestsInFlight.size===0,'STOP retained pending direction');
 const count=__requests.filter(r=>r.url.includes('move=FORWARD')).length;await __step(2000);__check(__wire.motion==='STOP'&&mode==='MANUAL'&&!controlRequestUncertain&&__requests.filter(r=>r.url.includes('move=FORWARD')).length===count,'late direction response undid STOP');
`);
await scenario('expired_manual_session_requires_fresh_press_only', manualServer+`
 startMove('FORWARD');await __step(1000);renderTelemetry({...__fixture,mode:'MANUAL',controlSessionAlive:false});await __step(50);
 __check(__wire.motion==='STOP'&&!activePress&&controlSessionToken&&!controlRequestUncertain,'session replacement did not retire old motion');
 startMove('LEFT');await __step(1000);__check(activePress&&__wire.motion==='LEFT'&&__modes().length===1,'session recovery required redundant Manual selection');stopMove();await __step(50);
`);
await scenario('healthy_release_before_direction_ack', `lastHeartbeatSuccess=lastTelemetrySuccess=Date.now();startMove('FORWARD');const old=__requests.find(r=>r.url.includes('move=FORWARD'));stopMove();await __settle();const release=__requests.find(r=>r.url.includes('move=RELEASE'));__ack(release);await __settle();renderTelemetry(__fixture);refreshConnectionBadge();__check(byId('connectionState').textContent.includes('Live')&&mode==='MANUAL'&&__fixture.safetyState==='SAFE','owner state absent');__check(!controlRequestUncertain&&[...document.querySelectorAll('.hold')].every(b=>!b.disabled),'healthy RELEASE latched controls inhibited');old.reject(Error('obsolete direction failure'));await __settle();startMove('LEFT');__check(activePress&&__requests.filter(r=>r.url.includes('move=LEFT')).length===1&&__modes().length===0,'next fresh press required rearming');`);
await scenario('normal_stop_then_auto', `setMode('AUTO');__check(__modes().length===0,'mode before STOP');__ack(__stops()[0]);await __settle();__check(__modes().length===1&&__modes()[0].url.includes('fence='),'missing fenced mode');__ack(__modes()[0]);await __settle();__check(mode==='AUTO'&&!modeTransition&&!controlRequestUncertain,'AUTO acknowledgement');`);
await scenario('telemetry_during_transition', `setMode('AUTO');await __settle();loadData();const stale=__requests.filter(r=>r.url.startsWith('/data')).at(-1);__check(stale,'transition telemetry absent');__ack(__stops()[0]);await __settle();__ack(__modes()[0]);await __settle();__ack(stale,{...__fixture,mode:'MANUAL'});await __settle();__check(mode==='AUTO','transition telemetry restored obsolete mode');`);
await scenario('auto_to_manual', `mode='AUTO';setMode('MANUAL');__check(!__stops()[0].url.includes('retire='),'AUTO STOP must be global');__ack(__stops()[0]);await __settle();__ack(__modes()[0]);await __settle();__check(mode==='MANUAL','MANUAL acknowledgement');`);
await scenario('multiple_stops', `explicitStop();setMode('AUTO');__ack(__stops()[1]);await __settle();__check(__modes().length===0,'older STOP bypassed');__ack(__stops()[0]);await __settle();__check(__modes().length===1,'mode missing');`);
await scenario('repeated_clicks', `const first=setMode('AUTO');const second=setMode('MANUAL');__check(first===second&&__stops().length===1,'overlapping transactions');__ack(__stops()[0]);await __settle();__check(__modes().length===1&&__modes()[0].url.includes('AUTO'),'queued click replay');`);
for (const failure of ['reject','timeout','abort']) {
  await scenario('stop_'+failure, `__ignoreAbort=true;setMode('AUTO');const old=__stops()[0];
    ${failure==='reject'?"old.reject(Error('Wi-Fi lost'));":failure==='abort'?"old.options.signal.dispatchEvent(new Event('abort'));":"await __advance(STOP_REQUEST_TIMEOUT_MS+1);"}
    await __settle();__check(!modeTransition&&controlRequestUncertain&&__modes().length===0,'STOP failure escaped barrier');
    __ack(old);await __settle();__check(__modes().length===0&&controlRequestUncertain,'late STOP replay');
    __check(!byId('stopBtn').disabled&&!activePress&&!pendingMove,'STOP accessibility or stale intent');`);
}
await scenario('late_stop_failure', `__ignoreAbort=true;setMode('AUTO');const old=__stops()[0];await __advance(STOP_REQUEST_TIMEOUT_MS+1);old.reject(Error('late failure'));await __settle();__check(__modes().length===0&&controlRequestUncertain,'late failure changed state');`);
await scenario('stop_rejection_status', `setMode('AUTO');__stops()[0].resolve({ok:false,status:409});await __settle();__check(__modes().length===0&&controlRequestUncertain&&!byId('modeTransitionStatus').hidden,'rejection invisible');`);
await scenario('missing_fence', `setMode('AUTO');__stops()[0].resolve({ok:true,status:200,headers:new Headers()});await __settle();__check(__modes().length===0&&controlRequestUncertain,'unfenced mode admitted');`);
await scenario('explicit_stop_cancels_wait', `setMode('AUTO');explicitStop();for(const r of __stops())__ack(r);await __settle();__check(__modes().length===0&&controlRequestUncertain,'STOP did not cancel mode');`);
for (const failure of ['reject','timeout','abort']) {
  await scenario('mode_'+failure, `__ignoreAbort=true;setMode('AUTO');__ack(__stops()[0]);await __settle();const old=__modes()[0];
    ${failure==='reject'?"old.resolve({ok:false,status:409});":failure==='abort'?"old.options.signal.dispatchEvent(new Event('abort'));":"await __advance(MODE_REQUEST_TIMEOUT_MS+1);"}
    await __settle();__check(controlRequestUncertain&&!modeTransition&&__stops().length===2,'mode failure did not fail closed');
    __ack(old);await __settle();__check(mode==='MANUAL'&&controlRequestUncertain&&__modes().length===1,'late mode restored obsolete controls');`);
}
await scenario('stop_cancels_sent_mode', `__ignoreAbort=true;setMode('AUTO');__ack(__stops()[0]);await __settle();const old=__modes()[0];explicitStop();__ack(old);await __settle();__check(mode==='MANUAL'&&controlRequestUncertain&&__stops().length===2,'late AUTO response survived STOP');`);
await scenario('manual_intent_retired', `startMove('FORWARD');__check(activePress,'manual press absent');setMode('AUTO');await __settle();__check(!activePress&&!activeMove&&!pendingMove,'manual intent retained');await __advance(3000);__check(__modes().length===0&&__requests.filter(r=>r.url.includes('move=FORWARD')).length===1,'old drive replayed');`);
await scenario('direction_deadline', `__ignoreAbort=true;startMove('FORWARD');await __advance(MOVE_REQUEST_TIMEOUT_MS+1);__check(controlRequestUncertain&&!activePress&&!pendingMove,'timed-out movement retained');const n=__requests.length;await __advance(1000);__check(__requests.slice(n).every(r=>!r.url.includes('move=FORWARD')),'automatic move retry');`);
await scenario('fresh_action_after_reconnect', `setMode('AUTO');__stops()[0].reject(Error('disconnect'));await __settle();const oldCount=__stops().length;setMode('AUTO');__check(__stops().length===oldCount,'retry before state confirmation');
    const data=__requests.filter(r=>r.url.startsWith('/data')).at(-1);__check(data,'fresh state request absent');__ack(data);await __settle();
    __check(recoveryStateConfirmed&&controlRequestUncertain&&__modes().length===0,'reconnect replayed mode');
    setMode('AUTO');__check(__stops().length===oldCount+1&&__modes().length===0,'fresh action bypassed STOP');`);
await scenario('fresh_telemetry_proves_node_responding', `lastHeartbeatSuccess=Date.now()-4000;lastTelemetrySuccess=Date.now();lastTelemetryNodeAgeMs=0;refreshConnectionBadge();__check(byId('connectionState').textContent.includes('Live'),'successful fresh telemetry incorrectly marked Offline');`);
await scenario('control_response_without_telemetry_is_stale', `lastHeartbeatSuccess=lastTelemetrySuccess=Date.now()-4000;recordRequest('cmd','OK',performance.now());refreshConnectionBadge();__check(byId('connectionState').textContent.includes('Stale'),'successful control response incorrectly marked Offline or fresh telemetry');`);
await scenario('no_response_remains_offline', `lastHeartbeatSuccess=lastTelemetrySuccess=Date.now()-4000;if(typeof lastNodeResponseSuccess!=='undefined')lastNodeResponseSuccess=Date.now()-4000;refreshConnectionBadge();__check(byId('connectionState').textContent.includes('Offline'),'real loss hidden');`);
await scenario('liveness_cannot_restore_control_authority', `inhibitControl('STOP not confirmed');lastHeartbeatSuccess=Date.now();lastTelemetrySuccess=Date.now();refreshConnectionBadge();__check(controlRequestUncertain&&[...document.querySelectorAll('.hold')].every(b=>b.disabled),'liveness authorized movement');`);
await scenario('stop_always_accessible', `setMode('AUTO');__check(!byId('stopBtn').disabled&&[...document.querySelectorAll('.hold')].every(b=>b.disabled),'transition controls unsafe');byId('stopBtn').dispatchEvent(new PointerEvent('pointerdown',{bubbles:true}));__check(__stops().length===2&&!modeTransition,'STOP listener missing');`);
await scenario('already_manual_ready', `__check(controlSessionToken&&!controlRequestUncertain&&[...document.querySelectorAll('.hold')].every(b=>!b.disabled),'fresh Manual not ready');setMode('MANUAL');await __settle();__check(__requests.length===0,'ready Manual reset mode/session');`);
await scenario('manual_refresh_and_temporary_stale', `const token=controlSessionToken;renderTelemetry(__fixture);updateConnectionState('stale');renderTelemetry(__fixture);updateConnectionState('live');__check(controlSessionToken===token&&!controlRequestUncertain&&__requests.length===0&&[...document.querySelectorAll('.hold')].every(b=>!b.disabled),'harmless refresh required rearming');`);
await scenario('one_click_mode_round_trip', `setMode('AUTO');__ack(__stops()[0]);await __settle();__ack(__modes()[0]);await __settle();renderTelemetry({...__fixture,mode:'AUTO'});setMode('MANUAL');__ack(__stops()[1]);await __settle();__ack(__modes()[1]);await __settle();__check(mode==='MANUAL'&&!controlRequestUncertain&&__stops().length===2&&__modes().length===2,'round trip required redundant transaction');`);
await scenario('no_background_session_during_mode', `setMode('AUTO');controlSessionToken=null;renderTelemetry({...__fixture,controlSessionAlive:false});__check(__requests.every(r=>r.url!=='/control-session'),'background RELEASE could revoke mode ticket');__ack(__stops()[0]);await __settle();__ack(__modes()[0]);await __settle();__check(mode==='AUTO'&&!controlRequestUncertain,'healthy transition failed');`);
await scenario('pending_session_then_mode', `controlSessionToken=null;ensureControlSession();const session=__requests.find(r=>r.url==='/control-session');setMode('AUTO');__ack(__stops()[0]);await __settle();__check(__modes().length===0,'mode overtook session RELEASE');__ack(session,{token:'old-session'});await __settle();__check(__stops().length===2&&controlSessionToken===null,'old session reply restored authority or final STOP absent');__ack(__stops()[1]);await __settle();__ack(__modes()[0]);await __settle();__check(mode==='AUTO'&&!controlRequestUncertain,'one-click transition did not finish');`);
await scenario('pending_session_timeout', `__ignoreAbort=true;controlSessionToken=null;ensureControlSession();const session=__requests.find(r=>r.url==='/control-session');setMode('AUTO');__ack(__stops()[0]);await __settle();await __advance(STOP_REQUEST_TIMEOUT_MS+1);__check(!modeTransition&&controlRequestUncertain&&__modes().length===0,'session timeout stranded transition or admitted mode');__ack(session,{token:'late-session'});await __settle();__check(controlSessionToken===null&&__modes().length===0,'late session revived obsolete authority');`);
await scenario('session_body_timeout', `controlSessionToken=null;ensureControlSession();const session=__requests.find(r=>r.url==='/control-session');session.resolve({ok:true,status:200,json:()=>new Promise(()=>{})});setMode('AUTO');__ack(__stops()[0]);await __settle();await __advance(STOP_REQUEST_TIMEOUT_MS+1);__check(!modeTransition&&controlRequestUncertain&&__modes().length===0,'session body left mode wait unbounded');`);
await scenario('real_release_failure_stays_safe', `startMove('FORWARD');stopMove();await __settle();__requests.find(r=>r.url.includes('move=RELEASE')).reject(Error('network lost'));await __settle();renderTelemetry(__fixture);__check(controlRequestUncertain&&!activePress&&!pendingMove&&[...document.querySelectorAll('.hold')].every(b=>b.disabled),'unknown RELEASE silently cleared');__check(!byId('stopBtn').disabled,'STOP inaccessible');`);
await scenario('unrelated_direction_abort_stays_safe', `startMove('FORWARD');moveAbortController.abort();await __settle();__check(controlRequestUncertain&&!activePress,'unexplained abort ignored');`);
await scenario('manual_priority_stop_remains_ready', `startMove('FORWARD');explicitStop();await __settle();__ack(__stops()[0]);await __settle();renderTelemetry(__fixture);__check(!activePress&&!pendingMove&&!controlRequestUncertain&&__modes().length===0,'successful explicit STOP caused rearming');`);
await scenario('reconnect_does_not_replay_motion', `startMove('FORWARD');__requests.find(r=>r.url.includes('move=FORWARD')).reject(Error('disconnect'));await __settle();const moves=__requests.filter(r=>r.url.includes('move=FORWARD')).length;renderTelemetry(__fixture);updateConnectionState('live');await __advance(1000);__check(controlRequestUncertain&&!activePress&&__requests.filter(r=>r.url.includes('move=FORWARD')).length===moves,'reconnection restarted old movement');`);
await scenario('offline_manual_recovers_without_mode_reselection', manualServer+`
 Date.now=()=>10000+__clock;
 startMove('FORWARD');await __step(1000);__dropAll=true;await __step(6000);
 __check(controlRequestUncertain&&!activePress&&!pendingMove&&__wire.motion==='STOP','outage did not stop and retire intent');
 const moves=__requests.filter(r=>r.url.includes('move=FORWARD')).length;
 __dropAll=false;__wire.sessionAlive=false;__wire.sequence+=20;
 pollPing();loadData();await __step(6500);
 __check(!controlRequestUncertain&&[...document.querySelectorAll('.hold')].every(b=>!b.disabled),'LIVE telemetry left Manual permanently inhibited');
 __check(__modes().length===1&&__wire.motion==='STOP'&&__requests.filter(r=>r.url.includes('move=FORWARD')).length===moves,'recovery replayed movement or mode');
 __responseDelay=20;startMove('LEFT');await __step(500);__check(__wire.motion==='LEFT','fresh press after reconnect failed');stopMove();await __step(50);
`);

for (const duration of [5000,10000,15000]) await scenario('repeated_offline_'+duration, manualServer+`
 Date.now=()=>10000+__clock;
 for(let cycle=0;cycle<3;cycle++){
  __responseDelay=20;startMove('FORWARD');await __step(500);__dropAll=true;await __step(${duration});
  const moves=__requests.filter(r=>r.url.includes('move=FORWARD')).length;
  __dropAll=false;__wire.sessionAlive=false;__wire.sequence+=100;pollPing();loadData();await __step(7000);
  __check(!controlRequestUncertain&&!activePress&&!pendingMove&&__wire.motion==='STOP','repeated outage failed safe recovery');
  __check(__modes().length===1&&__requests.filter(r=>r.url.includes('move=FORWARD')).length===moves,'repeated outage replayed mode/move');
 }
 startMove('BACKWARD');await __step(500);__check(__wire.motion==='BACKWARD','fresh direction after repeated outages');stopMove();
`);
for (const failure of ['reject','timeout','missing_authority']) await scenario('bounded_recovery_stop_'+failure, manualServer+`
 Date.now=()=>10000+__clock;inhibitControl('Command not confirmed');
 const serve=fetch;fetch=(url,options)=>{
  if(url.includes('move=STOP')){
   const p=originalFetch(url,options),r=__requests.at(-1);
   ${failure==='reject'?"setTimeout(()=>r.resolve({ok:false,status:409}),5);":failure==='missing_authority'?"setTimeout(()=>__ack(r),5);":""}
   return p;
  }return serve(url,options);
 };
 const before=__stops().length;pollPing();loadData();await __step(30000);
 __check(controlRequestUncertain&&controlRecoveryAttempts===3&&__stops().length-before===3,'recovery did not stop after bounded attempts');
 __check(!activePress&&!pendingMove&&[...document.querySelectorAll('.hold')].every(b=>b.disabled),'unknown STOP/authority enabled movement');
 __check(byId('modeTransitionStatus').textContent.includes('3 attempts'),'unresolved condition not reported');
`);
await scenario('recovery_does_not_take_other_client_session', manualServer+`
 Date.now=()=>10000+__clock;__wire.token='other-client';inhibitControl('Command not confirmed');
 const count=__requests.filter(r=>r.url==='/control-session').length;pollPing();loadData();await __step(5000);
 __check(controlRequestUncertain&&!automaticControlRecovery&&__requests.filter(r=>r.url==='/control-session').length===count,'recovery stole another client authority');
 __check(byId('modeTransitionStatus').textContent.includes('Another control session'),'other owner not reported');
`);
await scenario('recovery_revocation_is_not_expiry', manualServer+`
 Date.now=()=>10000+__clock;inhibitControl('Command not confirmed');const serve=fetch;
 fetch=(url,options)=>{if(url.includes('move=STOP')){const p=originalFetch(url,options),r=__requests.at(-1);r.controlHeaders={'X-PIKU-Control-State':'MANUAL_REVOKED','X-PIKU-Command-Sequence':'100'};setTimeout(()=>__ack(r),5);return p;}return serve(url,options);};
 pollPing();loadData();await __step(5000);
 __check(controlRequestUncertain&&!automaticControlRecovery&&__stops().length===2,'revocation was retried/cleared');
 __check(byId('modeTransitionStatus').textContent.includes('revoked'),'revocation not reported');
`);
await scenario('revoked_telemetry_cannot_create_session', `
 const count=__requests.filter(r=>r.url==='/control-session').length;
 renderTelemetry({...__fixture,controlSessionAlive:false,controlSessionRevoked:true});
 __check(controlRequestUncertain&&!automaticControlRecovery&&__requests.filter(r=>r.url==='/control-session').length===count,'revocation renewed session automatically');
`);
for(const hazard of ['flameCritical','gasCritical','temperatureCritical']) await scenario('recovery_safety_'+hazard, manualServer+`
 Date.now=()=>10000+__clock;inhibitControl('Command not confirmed');
 __fixture.${hazard}=true;__fixture.safetyState='CRITICAL';pollPing();loadData();await __step(5000);
 __check(controlRequestUncertain&&controlRecoveryAttempts===0&&__wire.motion==='STOP','critical hold cleared by reconnect');
 __fixture.${hazard}=false;__fixture.safetyState='SAFE';await __step(5000);
 __check(!controlRequestUncertain&&!activePress&&__wire.motion==='STOP','cleared hazard did not safely synchronize');
`);
await scenario('explicit_stop_during_recovery_stays_inhibited', manualServer+`
 Date.now=()=>10000+__clock;inhibitControl('Command not confirmed');loadData();await __step(25);
 inhibitControl('Command not confirmed');__dropAll=true;explicitStop();await __step(5000);
 __check(controlRequestUncertain&&!automaticControlRecovery&&!activePress&&__wire.motion==='STOP','explicit intervention automatically rearmed uncertain control');
`);
await scenario('auto_telemetry_after_interruption_never_submits_mode', manualServer+`
 Date.now=()=>10000+__clock;__wire.mode='AUTO';mode='AUTO';inhibitControl('STOP not confirmed');
 const count=__modes().length;pollPing();loadData();await __step(10000);
 __check(mode==='AUTO'&&__modes().length===count&&!activePress,'AUTO replayed or required automatic reselection');
`);
await scenario('stale_recovery_stop_after_explicit_stop', `
 inhibitControl('Command not confirmed');sendCmd('STOP',true);const old=__stops().at(-1);
 explicitStop();old.controlHeaders={'X-PIKU-Control-State':'MANUAL_AUTHORIZED','X-PIKU-Command-Sequence':'100'};__ack(old);await __settle();
 __check(controlRequestUncertain&&!automaticControlRecovery,'old recovery proof overrode newer STOP');
`);
await scenario('session_renewal_missing_sequence_stays_inhibited', `
 inhibitControl('Command not confirmed');sendCmd('STOP',true);const stop=__stops().at(-1);
 stop.controlHeaders={'X-PIKU-Control-State':'MANUAL_SESSION_EXPIRED','X-PIKU-Command-Sequence':'100'};__ack(stop);await __settle();
 const session=__requests.filter(r=>r.url==='/control-session').at(-1);__ack(session,{token:'renewed'});await __settle();
 __check(controlRequestUncertain&&!activePress,'session without sequence restored controls');
`);

await scenario('authorized_stop_without_sequence_is_not_proof', `
 inhibitControl('Command not confirmed');sendCmd('STOP',true);const stop=__stops().at(-1);stop.controlHeaders={'X-PIKU-Control-State':'MANUAL_AUTHORIZED'};__ack(stop);await __settle();
 __check(controlRequestUncertain,'authority without command sequence enabled controls');
`);
await scenario('stale_stop_after_revoked_telemetry_is_not_proof', `
 inhibitControl('Command not confirmed');sendCmd('STOP',true);const stop=__stops().at(-1);
 renderTelemetry({...__fixture,controlSessionAlive:false,controlSessionRevoked:true});
 stop.controlHeaders={'X-PIKU-Control-State':'MANUAL_AUTHORIZED','X-PIKU-Command-Sequence':'100'};__ack(stop);await __settle();
 __check(controlRequestUncertain&&!automaticControlRecovery,'stale STOP undid confirmed revocation');
`);
await scenario('stale_publisher_does_not_start_recovery', manualServer+`
 Date.now=()=>10000+__clock;inhibitControl('Command not confirmed');__fixture.telemetry_age_ms=5000;pollPing();loadData();await __step(5000);
 __check(controlRequestUncertain&&controlRecoveryAttempts===0,'stale publisher state started control recovery');
 __fixture.telemetry_age_ms=0;await __step(5000);__check(!controlRequestUncertain&&!activePress,'fresh publisher did not allow safe recovery');
`);
// End-to-end browser transactions through the current extracted C++ handlers.
if(input.nativeEndpoint) {
 const nativeServer = `
 Date.now=()=>10000+__clock;
 window.__nativePending=new Set();window.__nativeDrop=false;
 const capturedFetch=fetch;
 window.__nativeGet=async path=>{const response=await __networkFetch(path+(path.includes('?')?'&':'?')+'now='+(10000+__clock));return response.json();};
 fetch=(url,options={})=>{
  const promise=capturedFetch(url,options),request=__requests.at(-1);
  if(__nativeDrop)return promise;
  const task=__networkFetch(url+(url.includes('?')?'&':'?')+'now='+(10000+__clock),{...options,signal:undefined})
   .then(async response=>{const body=await response.json();request.resolve({ok:response.ok,status:response.status,headers:response.headers,
    json:()=>Promise.resolve(url.startsWith('/data')?{...__fixture,...body}:body)});},error=>request.reject(error));
  __nativePending.add(task);task.finally(()=>__nativePending.delete(task));return promise;
 };
 window.__drainNative=async()=>{for(let round=0;round<50;round++){await Promise.all([...__nativePending]);await __settle();if(!__nativePending.size)return;}throw Error('native transaction did not settle');};
 mode='AUTO';renderTelemetry({...__fixture,mode:'AUTO'});setMode('MANUAL');await __drainNative();
 __check(mode==='MANUAL'&&controlSessionToken&&!controlRequestUncertain,'native initial Manual failed');
 `;
 for(const duration of [5000,10000,15000]) await scenario('native_handlers_reconnect_'+duration, nativeServer+`
  for(let cycle=0;cycle<3;cycle++){
   startMove('FORWARD');await __drainNative();const accepted=await __nativeGet('/state');__check(accepted.accepted==='FORWARD','native direction admission failed');
   __nativeDrop=true;await __advance(${duration});await __settle();
   __check(controlRequestUncertain&&!activePress&&!pendingMove,'native outage retained held intent');
   const moves=__requests.filter(r=>r.url.includes('move=FORWARD')).length;
   __nativeDrop=false;pollPing();loadData();await __drainNative();
   // Existing in-flight poll deadlines may need to retire before next telemetry.
   for(let step=0;step<6&&controlRequestUncertain;step++){await __advance(1000);await __drainNative();}
   const stopped=await __nativeGet('/state');
   __check(!controlRequestUncertain&&stopped.accepted==='RELEASE'&&__modes().length===1,'native session RELEASE did not recover without mode reselection');
   __check(__requests.filter(r=>r.url.includes('move=FORWARD')).length===moves&&!activePress&&!pendingMove,'native reconnect replayed movement');
  }
  startMove('LEFT');await __drainNative();__check((await __nativeGet('/state')).accepted==='LEFT','native fresh press failed');stopMove();await __drainNative();
 `);
 await scenario('native_handlers_sequence_jump', nativeServer+`
  __nativeDrop=true;startMove('FORWARD');await __advance(6000);await __settle();
  await __nativeGet('/cmd?move=STOP&seq=900');await __nativeGet('/expire');
  __nativeDrop=false;pollPing();loadData();await __drainNative();
  for(let step=0;step<6&&controlRequestUncertain;step++){await __advance(1000);await __drainNative();}
  __check(!controlRequestUncertain&&(await __nativeGet('/state')).sequence>900,'native changed sequence/session failed recovery');
  startMove('RIGHT');await __drainNative();__check((await __nativeGet('/state')).accepted==='RIGHT','native fresh command sequence stale');stopMove();await __drainNative();
 `);
 await scenario('native_handlers_delayed_auto_after_stop', nativeServer+`
  const ticket=await __nativeGet('/cmd?move=STOP&seq=100&token='+controlSessionToken);
  explicitStop();await __drainNative();
  const late=await __nativeGet('/mode?set=AUTO&fence='+ticket.fence);
  __check(late.status===409&&late.mode==='MANUAL'&&!activePress,'native delayed AUTO escaped newer STOP');
 `);
 await scenario('native_handlers_revocation', nativeServer+`
  await __nativeGet('/revoke');inhibitControl('Command not confirmed');loadData();await __drainNative();
  __check(controlRequestUncertain&&!automaticControlRecovery&&(await __nativeGet('/state')).revoked,'native revocation silently reauthorized');
  setMode('MANUAL');await __drainNative();__check(!controlRequestUncertain&&!(await __nativeGet('/state')).revoked&&controlSessionToken,'fresh operator Manual authorization failed');
 `);
 await scenario('native_handlers_two_clients', nativeServer+`
  await __nativeGet('/other');inhibitControl('Command not confirmed');loadData();await __drainNative();
  const owner=await __nativeGet('/state');__check(controlRequestUncertain&&!automaticControlRecovery&&owner.token!==controlSessionToken,'native recovery stole second client session');
 `);
}
// Render every panel at the owner's requested widths, four landscape sizes,
// 200% text, and 125% CSS zoom. The original markup identities are retained.
const typographySelectors = ['.metric-label','.metric-note','.metric-value','.mini-metric span',
  '.mini-metric strong','.detail-item span','.detail-item strong','.control-readouts span',
  '.system-summary span','.status-badge','.panel-badge','.alarm-label','.alarm-detail',
  '.control-note','.sensor-head-btn','.sensor-head-status','.sensor-head-availability',
  '.nav-btn','.dpad button','.eyebrow','.wifi-form label','.system-note'];
const viewports = [320,360,390,430,768,1024,1440,1920].map(width=>({width,height:width<700?800:1000}));
viewports.push(...[{width:740,height:320},{width:800,height:360},{width:844,height:390},{width:932,height:430},
  {width:320,height:800,text:2},{width:1440,height:1000,text:2},{width:1440,height:1000,zoom:1.25}]);
await reset();
results.originalIds = await evaluate(`Array.from(document.querySelectorAll('[id]'),e=>e.id)`);
const baselineIds = [...asset(input.baseline,'html').matchAll(/\bid="([^"]+)"/g)].map(m=>m[1]);
for (const id of baselineIds) if(!results.originalIds.includes(id)) throw Error('Original DOM id removed: '+id);
const duplicates = results.originalIds.filter((id,i,a)=>a.indexOf(id)!==i);
if(duplicates.length) throw Error('Duplicate DOM ids: '+duplicates);
for (const vp of viewports) {
  await call('Emulation.setDeviceMetricsOverride',{width:vp.width,height:vp.height,deviceScaleFactor:1,mobile:false});
  await evaluate(`document.documentElement.style.fontSize='${(vp.text||1)*100}%';document.body.style.zoom='${vp.zoom||1}';`);
  for (const page of ['overview','control','sensors','communication','system']) {
    const observed = await evaluate(`(()=>{
      activateSection('${page}',false);renderTelemetry(__fixture);
      const problems=[];
      if(document.documentElement.scrollWidth>innerWidth+2)problems.push('document overflow');
      const visible=e=>e.getClientRects().length&&getComputedStyle(e).visibility!=='hidden';
      for(const e of document.querySelectorAll('button,strong,.metric-label,.metric-note,.eyebrow,.control-note,.system-note,.detail-item span')){
        if(!visible(e))continue;const r=e.getBoundingClientRect();
        if(r.left < -2 || r.right > innerWidth+2)problems.push('horizontal bounds: '+(e.id||e.className));
        if(e.clientWidth&&e.scrollWidth>e.clientWidth+2)problems.push('clipped text: '+(e.id||e.className));
        if(e.clientHeight&&e.scrollHeight>e.clientHeight+2)problems.push('vertical text: '+(e.id||e.className));
      }
      for(const card of document.querySelectorAll('.metric-card')){
        if(!visible(card))continue;const value=card.querySelector('.metric-value').getBoundingClientRect();
        const label=card.querySelector('.metric-label').getBoundingClientRect(),note=card.querySelector('.metric-note').getBoundingClientRect();
        if(label.bottom>value.top+1||value.bottom>note.top+1)problems.push('metric overlap');
      }
      const fonts=Object.fromEntries(${JSON.stringify(typographySelectors)}.map(selector=>{
        const e=[...document.querySelectorAll(selector)].find(visible);return [selector,e?parseFloat(getComputedStyle(e).fontSize):null];
      }));
      for(const [selector,size]of Object.entries(fonts))if(size!==null&&size<14)problems.push('small text '+selector+': '+size);
      const nav=[...document.querySelectorAll('.nav-btn')].map(e=>({text:e.textContent,width:e.getBoundingClientRect().width,height:e.getBoundingClientRect().height}));
      if(nav.some(e=>e.width<44||e.height<44))problems.push('small navigation target');
      if('${page}'==='control'){
        const stop=byId('stopBtn');stop.scrollIntoView({block:'center'});const r=stop.getBoundingClientRect();
        const hit=document.elementFromPoint(r.left+r.width/2,r.top+r.height/2);
        if(stop.disabled||!hit||hit.closest('button')!==stop)problems.push('STOP obscured');
      }
      return {problems,fonts,nav,errors:__errors};
    })()`);
    results.layout.push({viewport:vp,page,...observed});
    if(observed.problems.length||observed.errors.length) throw Error(JSON.stringify(results.layout.at(-1)));
  }
}
// Before/after computed values come from the actual embedded CSS, not declarations.
results.fontComparison = {};
for (const variant of ['baseline','candidate']) {
  await reset(variant==='baseline'?input.baseline:input.assets);
  await call('Emulation.setDeviceMetricsOverride',{width:390,height:844,deviceScaleFactor:1,mobile:false});
  results.fontComparison[variant] = await evaluate(`(()=>{
    activateSection('control',false);renderTelemetry(__fixture);
    return Object.fromEntries(${JSON.stringify(typographySelectors)}.map(s=>{const e=document.querySelector(s);return [s,e?parseFloat(getComputedStyle(e).fontSize):null];}));
  })()`);
  if(input.output){
    const capture=await call('Page.captureScreenshot',{format:'png',captureBeyondViewport:false});
    await Deno.writeFile(input.output+'/'+variant+'-control-390.png',Uint8Array.from(atob(capture.data),c=>c.charCodeAt(0)));
    await evaluate(`byId('stopBtn').scrollIntoView({block:'center'});`);
    const stopCapture=await call('Page.captureScreenshot',{format:'png',captureBeyondViewport:false});
    await Deno.writeFile(input.output+'/'+variant+'-stop-390.png',Uint8Array.from(atob(stopCapture.data),c=>c.charCodeAt(0)));
  }
}
results.canvasLabels = await evaluate(`(()=>{
  const canvas=byId('radar'),ctx=canvas.getContext('2d'),labels=[];
  const original=ctx.fillText;ctx.fillText=function(text,x,y){
    const font=parseFloat(this.font.match(/([\\d.]+)px/)[1]),m=this.measureText(text);
    const left=this.textAlign==='right'?x-m.width:this.textAlign==='center'?x-m.width/2:x;
    labels.push({text,fontCss:font*canvas.clientWidth/canvas.width,left,right:left+m.width,top:y-m.actualBoundingBoxAscent,bottom:y+m.actualBoundingBoxDescent});
    return original.call(this,text,x,y);
  };
  drawRadar(__fixture);ctx.fillText=original;
  __check(labels.length===7,'radar labels missing');
  for(const label of labels)__check(label.fontCss>=13.99&&label.left>=0&&label.right<=canvas.width&&label.top>=0&&label.bottom<=canvas.height,'radar text clipped');
  return labels;
})()`);
if(input.output){
  await call('Emulation.setDeviceMetricsOverride',{width:1440,height:1000,deviceScaleFactor:1,mobile:false});
  await evaluate(`activateSection('overview',false);window.scrollTo(0,0);`);
  const capture=await call('Page.captureScreenshot',{format:'png',captureBeyondViewport:false});
  await Deno.writeFile(input.output+'/candidate-overview-1440.png',Uint8Array.from(atob(capture.data),c=>c.charCodeAt(0)));
}
ws.send(JSON.stringify({id:++id,method:'Browser.close'}));
await new Promise(resolve=>setTimeout(resolve,200));
ws.close();
console.log(JSON.stringify(results));
