#pragma once
#include <pgmspace.h>

// Configuration page, Italian and English. The language follows the phone and can be
// switched with the button in the header; the choice is remembered in the browser.
// The browser computes the schedule preview with the same formula as lib/schedule,
// so the microcontroller does no extra work.
static const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Drinkuino</title>
<style>
body{font-family:system-ui,sans-serif;margin:0 auto;padding:16px;max-width:480px;background:#f4f8fb;color:#123}
header{display:flex;align-items:center;justify-content:space-between;margin:.2em 0 .5em}
h1{font-size:1.4em;margin:0}
fieldset{border:1px solid #cde;border-radius:10px;margin:0 0 14px;padding:10px 12px;background:#fff}
legend{font-weight:600;padding:0 4px}
label{display:block;margin:8px 0 2px;font-size:.9em}
input{width:100%;box-sizing:border-box;padding:9px;font-size:1em;border:1px solid #bcd;border-radius:8px}
.row{display:flex;gap:10px}.row>div{flex:1}
button{width:100%;padding:13px;font-size:1.1em;border:0;border-radius:10px;background:#1673d0;color:#fff;font-weight:600}
#lg{width:auto;padding:6px 14px;font-size:.9em;border-radius:20px;background:#e3f0fc;color:#1673d0}
#pv{line-height:1.6}#st{margin:0 0 12px;color:#345}
.t{display:inline-block;background:#e3f0fc;border-radius:6px;padding:0 6px;margin:2px}
.w{color:#b3261e}small{color:#567}
</style></head><body>
<header><h1>&#x1F4A7; Drinkuino</h1><button type="button" id="lg" aria-label="Language / Lingua">IT</button></header>
<div id="st"></div>
<form method="post" action="/save" id="f">
<fieldset><legend data-t="wifi"></legend>
<label data-t="net"></label><input name="ssid" id="ssid" list="nets" autocomplete="off"><datalist id="nets"></datalist>
<label>Password</label><input name="pass" type="password" data-p="pwph">
<small data-t="wifinote"></small>
</fieldset>
<fieldset><legend data-t="you"></legend><div class="row">
<div><label data-t="w"></label><input name="w" id="w" type="number" min="30" max="250" required></div>
<div><label data-t="h"></label><input name="h" id="h" type="number" min="100" max="230"></div>
<div><label data-t="age"></label><input name="age" id="age" type="number" min="1" max="120" required></div>
</div></fieldset>
<fieldset><legend data-t="day"></legend><div class="row">
<div><label data-t="wake"></label><input name="wake" id="wake" type="time" required></div>
<div><label data-t="sleep"></label><input name="sleep" id="sleep" type="time" required></div>
</div><div class="row">
<div><label data-t="glass"></label><input name="glass" id="glass" type="number" min="100" max="1000" step="10" required></div>
<div><label data-t="ovr"></label><input name="ovr" id="ovr" type="number" min="1000" max="6000" step="50" data-p="opt"></div>
</div></fieldset>
<fieldset><legend data-t="pvt"></legend><div id="pv"></div></fieldset>
<input type="hidden" name="epoch" id="epoch"><input type="hidden" name="l" id="l">
<button data-t="save"></button>
</form>
<script>
const T={
it:{wifi:"WiFi (solo per l'ora, una volta al giorno)",net:'Rete',pwph:'vuota = invariata',
wifinote:"Facoltativa: l'ora la prendo anche da questo telefono.",you:'Tu',w:'Peso (kg)',h:'Altezza (cm)',
age:'Età',day:'Giornata',wake:'Sveglia',sleep:'A letto',glass:'Bicchiere (ml)',ovr:'Obiettivo del medico (ml)',
opt:'facoltativo',pvt:'Anteprima',save:'Salva e avvia',fill:'Compila i campi per vedere gli orari.',
perday:'al giorno',of:'bicchieri da',every:'uno ogni ~',doc:'(obiettivo del medico)',
many:'Troppi bicchieri per la giornata: usa un bicchiere più grande.',
adapt:'Gli orari si adattano: snooze, bicchieri in anticipo o saltati spostano i successivi.',
batt:'Batteria',today:'oggi',yday:'ieri'},
en:{wifi:'WiFi (only for the clock, once a day)',net:'Network',pwph:'empty = unchanged',
wifinote:'Optional: I can also take the time from this phone.',you:'You',w:'Weight (kg)',h:'Height (cm)',
age:'Age',day:'Your day',wake:'Wake up',sleep:'Bedtime',glass:'Glass (ml)',ovr:"Doctor's target (ml)",
opt:'optional',pvt:'Preview',save:'Save and start',fill:'Fill in the fields to see the times.',
perday:'per day',of:'glasses of',every:'one every ~',doc:"(doctor's target)",
many:'Too many glasses for your day: use a bigger glass.',
adapt:'Times adapt: snoozes, early or missed glasses shift the following ones.',
batt:'Battery',today:'today',yday:'yesterday'}};
const $=i=>document.getElementById(i),F=['w','h','age','wake','sleep','glass','ovr'];
let L,C=null;
try{L=localStorage.getItem('lang')}catch(e){}
if(L!=='it'&&L!=='en')L=(navigator.language||'').toLowerCase().startsWith('it')?'it':'en';
const t=k=>T[L][k];
const num=(x,d)=>{const s=x.toFixed(d);return L==='it'?s.replace('.',','):s};
const hm=s=>{const p=s.split(':');return +p[0]*60+ +p[1]};
const f2=n=>String(n).padStart(2,'0');
const fmt=m=>{m=Math.round(m)%1440;return f2(m/60|0)+':'+f2(m%60)};
function target(w,h,a,o){if(o>0)return o;let x=w;if(h>=100){const h2=(h/100)**2,r=25*h2;if(w/h2>30)x=r+.4*(w-r)}
return Math.round(Math.min(4500,Math.max(2500,x*(a>55?30:35))))}
function pv(){
const w=+$('w').value,h=+$('h').value,a=+$('age').value,o=+$('ovr').value,g=+$('glass').value;
if(!w||!g||!$('wake').value||!$('sleep').value){$('pv').textContent=t('fill');return}
const tg=target(w,h,a,o),n=Math.min(40,Math.ceil(tg/g)),wk=hm($('wake').value),s=wk+15;
let e=hm($('sleep').value);if(e<=wk)e+=1440;
let st=n>1?(e-s)/(n-1):0,warn='';
if(n>1&&st<20){st=20;warn='<p class="w">'+t('many')+'</p>'}
const ts=[];for(let i=0;i<n&&s+i*st<=e;i++)ts.push(s+i*st);
$('pv').innerHTML='<b>'+num(tg/1000,2)+' L</b> '+t('perday')+' = <b>'+n+'</b> '+t('of')+' '+g+' ml'
+(n>1?', '+t('every')+Math.round(st)+' min':'')+(o>0?' <small>'+t('doc')+'</small>':'')+'<br>'
+ts.map(m=>'<span class="t">'+fmt(m)+'</span>').join('')+warn+'<br><small>'+t('adapt')+'</small>'}
function status(){
if(!C){$('st').textContent='';return}
const v=C.vbat/1000,pc=Math.max(0,Math.min(100,Math.round((v-3.3)/.85*100)));
$('st').innerHTML=t('batt')+' '+num(v,2)+' V (~'+pc+'%)'
+(C.configured?' &middot; '+t('today')+' '+C.today+'/'+C.goal+(C.ygoal?' &middot; '+t('yday')+' '+C.yday+'/'+C.ygoal:''):'')}
function apply(){
document.documentElement.lang=L;
document.querySelectorAll('[data-t]').forEach(e=>e.textContent=t(e.dataset.t));
document.querySelectorAll('[data-p]').forEach(e=>e.placeholder=t(e.dataset.p));
$('lg').textContent=L==='it'?'EN':'IT';$('l').value=L;status();pv()}
$('lg').addEventListener('click',()=>{L=L==='it'?'en':'it';try{localStorage.setItem('lang',L)}catch(e){}apply()});
F.forEach(k=>$(k).addEventListener('input',pv));
$('f').addEventListener('submit',()=>{$('epoch').value=Date.now();$('f').action='/save?l='+L});
apply();
fetch('/cfg.json').then(r=>r.json()).then(c=>{
C=c;const d={w:70,h:175,age:40,wake:'07:00',sleep:'23:00',glass:250,ovr:''};
F.forEach(k=>$(k).value=c.configured?c[k]:d[k]);
if(!c.ovr)$('ovr').value='';
$('ssid').value=c.ssid||'';
(c.nets||[]).forEach(n=>{const o=document.createElement('option');o.value=n;$('nets').appendChild(o)});
status();pv()}).catch(pv);
</script></body></html>)HTML";

// Shown after saving: English by default, Italian when the form was sent with l=it.
static const char SAVED_PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Drinkuino</title>
<style>body{font-family:system-ui,sans-serif;max-width:480px;margin:0 auto;padding:24px;background:#f4f8fb;color:#123}</style>
</head><body><h1>&#x1F4A7; <span id="a">Saved</span></h1>
<p id="b">Drinkuino is restarting and will remind you to drink from now on. You can close this page.</p>
<p><small id="c">To come back here: hold <b>Zz</b> for 5 seconds.</small></p>
<script>if(/[?&]l=it/.test(location.search)){document.documentElement.lang='it';
document.getElementById('a').textContent='Salvato';
document.getElementById('b').textContent='Drinkuino si riavvia e da ora ti ricorda di bere. Puoi chiudere questa pagina.';
document.getElementById('c').innerHTML='Per tornare qui: tieni premuto <b>Zz</b> per 5 secondi.'}</script>
</body></html>)HTML";
