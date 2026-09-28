const fs=require('fs'),vm=require('vm'),assert=require('assert');
const html=fs.readFileSync(__dirname+'/preview.html','utf8');let now=0,tick,draws=0;
const context=new Proxy({drawImage(){draws++;}},{get:(o,k)=>o[k]||(()=>{}),set:(o,k,v)=>(o[k]=v,true)});
const el=()=>({value:'',textContent:'',innerHTML:'',append(){},getContext(){return context;}}),ids={};
for(const id of ['play','state','speed','scrub','prev','next','frameLabel','status','grid'])ids[id]=el();ids.state.value='walk';ids.speed.value='1';
const env={document:{getElementById:id=>ids[id],createElement:el},window:{addEventListener(){}},performance:{now:()=>now},setInterval:f=>{tick=f;},Image:class{set src(s){assert(s.startsWith('data:image/png;base64,'));this.complete=true;this.naturalWidth=1024;this.onload();}}};
vm.runInNewContext(html.match(/<script>([\s\S]*?)<\/script>/)[1],env);assert.equal(draws,8);now=100;tick();assert.notEqual(ids.frameLabel.textContent,'1 / 16');ids.play.onclick();const paused=ids.frameLabel.textContent;now=200;tick();assert.equal(ids.frameLabel.textContent,paused);ids.next.onclick();tick();assert.notEqual(ids.frameLabel.textContent,paused);
for(const [state,n] of [['attack',24],['projectile',12],['hero-impact',12],['ground-impact',16]]){ids.state.value=state;ids.state.onchange();tick();assert.equal(ids.frameLabel.textContent,'1 / '+n);ids.scrub.value=String(n-1);ids.scrub.oninput();tick();assert.equal(ids.frameLabel.textContent,n+' / '+n);}
console.log('PASS: embedded images, all action modes, advancing frames, pause, stepping and scrubbing. This is a minimal-DOM check, not visual browser validation.');
const m=JSON.parse(fs.readFileSync(__dirname+'/atlas-manifest.json','utf8'));
assert.equal(m.sheets.length,27);assert.equal(ids.status.textContent,'27 / 27 sheets loaded');
for(const e of m.entries.filter(e=>e.state==='attack')){assert.equal(e.frames.length,24);assert(e.frames[11].sourceFile.startsWith('attack-a-'));assert(e.frames[12].sourceFile.startsWith('attack-b-'));}
assert.equal(m.entries.find(e=>e.state==='ground-impact').frames[0].sourcePose,1);
