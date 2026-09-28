const fs=require('fs'),vm=require('vm'),assert=require('assert');
const html=fs.readFileSync(__dirname+'/preview.html','utf8');let now=0,tick,draws=0;
const context=new Proxy({drawImage(){draws++;}},{get:(o,k)=>o[k]||(()=>{}),set:(o,k,v)=>(o[k]=v,true)});
const el=()=>({value:'',textContent:'',innerHTML:'',append(){},getContext(){return context;}}),ids={};
for(const id of ['play','state','speed','scrub','prev','next','frameLabel','status','grid'])ids[id]=el();ids.state.value='walk';ids.speed.value='1';
const env={document:{getElementById:id=>ids[id],createElement:el},window:{addEventListener(){}},performance:{now:()=>now},setInterval:f=>{tick=f;},Image:class{set src(s){assert(s.startsWith('data:image/png;base64,'));this.complete=true;this.naturalWidth=1024;this.onload();}}};
vm.runInNewContext(html.match(/<script>([\s\S]*?)<\/script>/)[1],env);assert.equal(draws,8);now=100;tick();assert.notEqual(ids.frameLabel.textContent,'1 / 16');ids.play.onclick();const paused=ids.frameLabel.textContent;now=200;tick();assert.equal(ids.frameLabel.textContent,paused);ids.next.onclick();tick();assert.notEqual(ids.frameLabel.textContent,paused);
for(const [state,n] of [['attack',24],['walk',16]]){ids.state.value=state;ids.state.onchange();tick();assert.equal(ids.frameLabel.textContent,'1 / '+n);ids.scrub.value=String(n-1);ids.scrub.oninput();tick();assert.equal(ids.frameLabel.textContent,n+' / '+n);}
const m=JSON.parse(fs.readFileSync(__dirname+'/atlas-manifest.json','utf8'));
assert.equal(m.sheets.length,24);
for(const e of m.entries.filter(e=>e.state==='attack')) {
  const seam=e.direction==='N'?8:e.direction==='NW'?11:12;
  assert(e.frames[seam-1].sourceFile.startsWith('attack-a-'));
  assert(e.frames[seam].sourceFile.startsWith('attack-b-'));
  assert.equal(e.frames.length,24);
}
assert.equal(ids.status.textContent,'24 / 24 sheets loaded');
console.log('PASS: embedded images, all action modes, advancing frames, pause, stepping and scrubbing. This is a minimal-DOM check, not visual browser validation.');
