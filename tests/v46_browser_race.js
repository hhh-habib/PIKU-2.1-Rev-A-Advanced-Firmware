// Actual embedded dashboard in Chromium; HTTP completion order is doubled.
const [socketUrl,inputPath]=Deno.args,input=JSON.parse(await Deno.readTextFile(inputPath));
const socket=new WebSocket(socketUrl);await new Promise((ok,bad)=>{socket.onopen=ok;socket.onerror=bad;});
let sequence=0;const pending=new Map();
socket.onmessage=e=>{const m=JSON.parse(e.data);if(!m.id)return;const p=pending.get(m.id);pending.delete(m.id);m.error?p.reject(m.error):p.resolve(m.result);};
const call=(method,params={})=>new Promise((resolve,reject)=>{const id=++sequence;pending.set(id,{resolve,reject});socket.send(JSON.stringify({id,method,params}));});
const evaluate=async expression=>{const r=await call('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});if(r.exceptionDetails)throw new Error(JSON.stringify(r.exceptionDetails));return r.result.value;};
const asset=delimiter=>input.assets.match(new RegExp('R"'+delimiter+'\\(([\\s\\S]*?)\\)'+delimiter+'";'))[1];
// Keep every original ordering assertion; supply the newly required STOP ticket.
const mock=`window.__requests=[];let fenceRevision=0;window.fetch=(url,options)=>new Promise((resolve,reject)=>window.__requests.push({url:String(url),options,resolve:response=>resolve({...response,headers:new Headers({'X-PIKU-Mode-Fence':'123.'+(++fenceRevision)})}),reject}));`;
await call('Page.enable');
const reset=async()=>{
 await call('Page.navigate',{url:'about:blank'});
 const tree=await call('Page.getFrameTree');
 const html=asset('html').replace('<link rel="stylesheet" href="/dashboard.css">','<style>'+asset('css')+'</style>').replace(/<script[^>]*src="\/dashboard.js"[^>]*><\/script>/,'<script>'+mock+'</script><script>'+asset('js')+'</script>');
 await call('Page.setDocumentContent',{frameId:tree.frameTree.frame.id,html});
 await evaluate(`(()=>{mode='MANUAL';loadData=()=>Promise.resolve();window.__requests=[];clearMoveRefresh();})()`);
};
const results={};
await reset();
results.ordered=await evaluate(`(async()=>{
 setMode('AUTO');
 const stop=window.__requests.find(r=>r.url.startsWith('/cmd?move=STOP'));
 const earlyMode=window.__requests.some(r=>r.url.startsWith('/mode?'));
 if(!stop)throw Error('retirement STOP absent');
 stop.resolve({ok:true});await new Promise(r=>setTimeout(r,0));
 return {earlyMode,maintenanceTagged:stop.url.includes('&retire=manual'),modeAfterStop:window.__requests.filter(r=>r.url.startsWith('/mode?')).length};
})()`);
if(results.ordered.earlyMode||!results.ordered.maintenanceTagged||results.ordered.modeAfterStop!==1)throw Error('V46-16: AUTO request raced ahead of classified retirement STOP completion');
await reset();
results.twoStops=await evaluate(`(async()=>{
 explicitStop();setMode('AUTO');
 const stops=window.__requests.filter(r=>r.url.startsWith('/cmd?move=STOP'));
 stops[1].resolve({ok:true});await new Promise(r=>setTimeout(r,0));
 const early=window.__requests.some(r=>r.url.startsWith('/mode?'));
 stops[0].resolve({ok:true});await new Promise(r=>setTimeout(r,0));
 return {early,modes:window.__requests.filter(r=>r.url.startsWith('/mode?')).length};
})()`);
if(results.twoStops.early||results.twoStops.modes!==1)throw Error('prior delayed STOP escaped the mode barrier');
await reset();
results.failedStop=await evaluate(`(async()=>{
 setMode('AUTO');window.__requests.find(r=>r.url.startsWith('/cmd?move=STOP')).reject(Error('transport failure'));
 await new Promise(r=>setTimeout(r,0));return window.__requests.some(r=>r.url.startsWith('/mode?'));
})()`);
if(results.failedStop)throw Error('failed retirement STOP must not admit a competing AUTO request');
await reset();
results.explicit=await evaluate(`(async()=>{
 setMode('AUTO');explicitStop();
 for(const r of window.__requests.filter(r=>r.url.startsWith('/cmd?move=STOP')))r.resolve({ok:true});
 await new Promise(r=>setTimeout(r,0));return {modes:window.__requests.filter(r=>r.url.startsWith('/mode?')).length,stops:window.__requests.filter(r=>r.url.startsWith('/cmd?move=STOP')).length};
})()`);
if(results.explicit.modes||results.explicit.stops!==2)throw Error('new genuine STOP failed to cancel waiting mode admission');
await reset();
results.lifecycle=await evaluate(`(async()=>{
 mode='AUTO';window.dispatchEvent(new Event('pagehide'));window.dispatchEvent(new Event('beforeunload'));
 Object.defineProperty(document,'hidden',{configurable:true,value:true});document.dispatchEvent(new Event('visibilitychange'));
 const stops=window.__requests.filter(r=>r.url.startsWith('/cmd?move=STOP'));if(stops.some(r=>r.url.includes('retire=')))throw Error('AUTO lifecycle incorrectly tagged');return stops.length;
})()`);
if(results.lifecycle!==3)throw Error('AUTO lifecycle STOP safety was removed or misclassified');
await reset();
results.manualLifecycle=await evaluate(`(()=>{
 window.dispatchEvent(new Event('pagehide'));window.dispatchEvent(new Event('beforeunload'));
 Object.defineProperty(document,'hidden',{configurable:true,value:true});document.dispatchEvent(new Event('visibilitychange'));
 explicitStop();const stops=window.__requests.filter(r=>r.url.startsWith('/cmd?move=STOP'));
 return stops.length===4&&stops.slice(0,3).every(r=>r.url.includes('&retire=manual'))&&!stops[3].url.includes('retire=');
})()`);
if(!results.manualLifecycle)throw Error('MANUAL lifecycle maintenance and genuine operator STOP were not distinguished');
socket.close();console.log(JSON.stringify(results));
