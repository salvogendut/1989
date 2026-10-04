import {mountShell} from './shell.js';
import {createKeyboard} from './keyboard.js';
import {createMedia,devices} from './media.js';

let core, media, running = false, starting = false, failed = false;
const $ = id => document.getElementById(id);
function status(message,error=false) { $('runStatus').textContent=message; $('runStatus').classList.toggle('error',error); }
const keyboard = createKeyboard(()=>running ? core : null);
mountShell({live:true,pressKey:keyboard.press,releaseInput:keyboard.release});
const slots = new Map(devices.map(id=>[id,document.querySelector(`[data-device="${id}"]`)]));
for (const [id,slot] of slots) {
  slot.querySelector('input').disabled=true;
  slot.querySelector('input').addEventListener('change',event=>{
    const file=event.target.files[0]; event.target.value='';
    if(file && media) media.load(id,file);
  });
  slot.querySelector('.eject-button')?.addEventListener('click',()=>media.eject(id));
  if (id !== 'cd') {
    const save=document.createElement('button');save.className='save-button';save.type='button';
    save.textContent='Download image';save.disabled=true;save.addEventListener('click',()=>media.download(id));slot.append(save);
  }
  if(id==='disk') {
    slot.querySelector('.write-state').textContent='SESSION COPY';
    const note=document.createElement('p');note.className='media-hint';note.textContent='Choose before starting the computer.';slot.append(note);
  }
}
function refresh() {
  for (const [id,slot] of slots) {
    const selection=media?.selections.get(id), unavailable=!media || media.busy || starting;
    slot.querySelector('input').disabled=unavailable || (id==='disk' && running);
    const eject=slot.querySelector('.eject-button');if(eject) eject.disabled=unavailable || !selection?.inserted;
    const save=slot.querySelector('.save-button');if(save) save.disabled=unavailable || !selection;
    if(selection) {
      slot.querySelector('output').textContent=(selection.inserted ? '' : 'Ejected · ')+selection.name;
      slot.querySelector('output').title=selection.name;
    } else {slot.querySelector('output').textContent='No image selected';slot.querySelector('output').removeAttribute('title');}
    slot.classList.toggle('loaded',Boolean(selection?.inserted));
  }
  $('startButton').disabled=!media || media.busy || starting || running || failed;
}
async function start() {
  if(!core || running || starting || media.busy || failed) return;
  starting=true; refresh(); status('Starting the NeXT…');
  try {
    core.callMain([]);
    if(!core._web_ready()) throw new Error('The emulator could not initialize. Reload to retry.');
    running=true; document.documentElement.dataset.running='true';
    $('powerScreen').hidden=true; $('canvas').focus();
    media.sync(); status('Running · ROM monitor. Click the screen to use your keyboard and mouse.');
    setInterval(()=>media.sync(),500);
  } catch(error) { failed=true; $('startButton').textContent='Reload to retry'; status(error.message,true); }
  finally {starting=false;refresh();}
}
$('startButton').addEventListener('click',start);
$('canvas').addEventListener('pointerdown',()=>$('canvas').focus());
$('canvas').addEventListener('contextmenu',event=>event.preventDefault());

try {
  if(!crossOriginIsolated || typeof SharedArrayBuffer==='undefined') throw new Error('Threading is unavailable. Serve this build with web/serve.py or enable COOP/COEP headers over HTTPS.');
  core=await window.create1989({
    canvas:$('canvas'),noInitialRun:true,
    print:message=>console.info('[1989]',message),
    printErr:message=>console.warn('[1989]',message),
    onAbort:message=>status(`Emulator stopped: ${message}`,true)
  });
  media=createMedia(core,{isRunning:()=>running,status,refresh});
  $('startButton').textContent='Start NeXT'; refresh();
  status('Ready · load your images, then start. Guest disk changes stay in this tab until downloaded.');
} catch(error) {status(error.message,true);$('startButton').textContent='Unable to start';}
