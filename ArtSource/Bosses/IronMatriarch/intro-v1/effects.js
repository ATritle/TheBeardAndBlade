// Procedural preview embers; recreate in the existing UE intro controller.
(()=>{const stage=document.querySelector('.stage'),canvas=document.createElement('canvas');
canvas.style.cssText='position:absolute;inset:0;width:100%;height:100%;pointer-events:none;z-index:2';stage.append(canvas);
const c=canvas.getContext('2d');let start=0,raf=0,particles=[];
function begin(){cancelAnimationFrame(raf);start=performance.now();particles=[];canvas.width=1280;canvas.height=720;
 for(let i=0;i<100;i++)particles.push({x:70+(i*97)%1140,y:560+(i*13)%100,vx:((i*17)%100-50)*1.2,vy:-40-(i*29)%160,s:2+i%4,birth:i%2?1.5:2.4});
 function draw(now){const t=(now-start)/1000;c.clearRect(0,0,1280,720);
 if(!stage.classList.contains('reduced'))for(const p of particles){const age=t-p.birth;if(age<0||age>2.2)continue;c.globalAlpha=Math.max(0,1-age/2.2);c.fillStyle=['#ff562d','#ffc86c','#dfd9c8','#b58443'][p.s%4];c.fillRect(p.x+p.vx*age,p.y+p.vy*age+32*age*age,p.s,p.s);}
 c.globalAlpha=1;if(t<6)raf=requestAnimationFrame(draw);else c.clearRect(0,0,1280,720);
 }raf=requestAnimationFrame(draw);}
new MutationObserver(records=>{for(const r of records)if(r.attributeName==='class'&&stage.classList.contains('play'))begin();}).observe(stage,{attributes:true,attributeFilter:['class']});
})();
