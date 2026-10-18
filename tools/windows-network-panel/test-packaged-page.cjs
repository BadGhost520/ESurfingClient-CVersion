// Uses installed Edge and Node standard library; all network APIs are mocked.
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const {spawnSync} = require('node:child_process');
const {pathToFileURL} = require('node:url');
const root = path.resolve(process.argv[2]);
const edge = [process.env['ProgramFiles(x86)'], process.env.ProgramFiles]
  .filter(Boolean).map(p => path.join(p, 'Microsoft/Edge/Application/msedge.exe')).find(fs.existsSync);
if (!edge) throw Error('Edge unavailable: browser coverage did not run');
const mock = `<script>
window.testStale=false; window.testReady=true; window.testPosts=0;
window.fetch=async (url, options={})=>{
 if(options.method==='POST') window.testPosts++;
 const data=String(url).includes('traffic.json') ? {
 timestamp:Date.now()-(window.testStale?15000:0),hotspotControls:true,defaultId:'test',
 adapters:[{id:'test',name:'Test adapter',down:1024,up:512}],
 hotspotBridge:{url:'http://127.0.0.1:18991',origins:[location.origin],ready:window.testReady,message:'mock unavailable'}
 } : String(url).includes(':18991/') ? {token:'mock',busy:false,defaultId:'test',
 adapters:[{id:'test',state:'Off',capability:'Enabled',connected:true}]} : {};
 return new Response(JSON.stringify(data),{status:200,headers:{'Content-Type':'application/json'}});
};
window.addEventListener('load',()=>setTimeout(async()=>{
 const check=(ok,message)=>{if(!ok)throw Error(message)};
 try {
  const traffic=Alpine.$data(document.querySelector('#traffic-card'));
  const hotspot=Alpine.$data(document.querySelector('#hotspot-card'));
  clearInterval(traffic.timer); clearInterval(hotspot.timer);
  await traffic.refresh(); await hotspot.refresh();
  check(traffic.fresh && traffic.downloadText==='1.00 KiB/s','fresh speed');
  check(hotspot.canStart,'fresh hotspot');
  check(document.querySelector('#traffic-card').compareDocumentPosition(document.querySelector('#hotspot-card'))&4,'card order');
  window.testReady=false; await traffic.refresh(); await hotspot.refresh();
  check(traffic.fresh && !hotspot.online && !hotspot.canStart,'listener failure containment');
  window.testReady=true; window.testStale=true; await traffic.refresh(); await hotspot.refresh();
  await hotspot.change('start');
  check(!traffic.fresh && !hotspot.canStart && window.testPosts===0,'stale safety');
  window.testStale=false; await traffic.refresh(); await hotspot.refresh();
  check(traffic.fresh && hotspot.canStart,'recovery');
  document.documentElement.setAttribute('data-cloud-check','pass');
 }catch(error){document.documentElement.setAttribute('data-cloud-check','fail:'+error.message)}
},800));
</script>`;
const page = path.join(root, 'cloud-check.html');
const profile = fs.mkdtempSync(path.join(os.tmpdir(),'esurfing-edge-'));
try {
  fs.writeFileSync(page, fs.readFileSync(path.join(root,'index.html'),'utf8').replace(/<head[^>]*>/i, '$&'+mock));
  const result = spawnSync(edge,['--headless=new','--disable-gpu','--no-first-run',
    '--user-data-dir='+profile,'--dump-dom','--virtual-time-budget=12000',pathToFileURL(page).href],
    {encoding:'utf8',timeout:60000,maxBuffer:4*1024*1024,windowsHide:true});
  if(result.error) throw result.error;
  if(!/data-cloud-check="pass"/.test(result.stdout||'')) {
    throw Error('Browser check failed: '+((result.stdout||'').match(/data-cloud-check="[^"]*"/)||['no completion marker'])[0]);
  }
  console.log('PASS: packaged page, fresh speed, card order, bridge failure, stale controls and recovery; no real hotspot requests');
} finally {
  fs.rmSync(page,{force:true});
  try {fs.rmSync(profile,{recursive:true,force:true});} catch (_) {}
}
