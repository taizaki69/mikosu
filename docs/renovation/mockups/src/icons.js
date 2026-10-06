// Inline SVG icons for the mockups (simple strokes, drawn for this project) and shared helpers.
const ICONS = {
  back: '<path d="M15 5l-7 7 7 7"/>',
  mode: '<circle cx="12" cy="12" r="8"/><circle cx="12" cy="12" r="3" fill="currentColor"/>',
  mods: '<path d="M12 3l2.4 5.2L20 9l-4.2 3.9L17 19l-5-2.8L7 19l1.2-6.1L4 9l5.6-.8z"/>',
  random: '<rect x="4" y="4" width="16" height="16" rx="3"/><circle cx="9" cy="9" r="1.2" fill="currentColor"/><circle cx="15" cy="15" r="1.2" fill="currentColor"/><circle cx="15" cy="9" r="1.2" fill="currentColor"/><circle cx="9" cy="15" r="1.2" fill="currentColor"/>',
  options: '<circle cx="12" cy="12" r="3"/><path d="M12 2.5v3M12 18.5v3M2.5 12h3M18.5 12h3M5.3 5.3l2.1 2.1M16.6 16.6l2.1 2.1M5.3 18.7l2.1-2.1M16.6 7.4l2.1-2.1"/>',
  search: '<circle cx="11" cy="11" r="6.5"/><path d="M16 16l4.5 4.5"/>',
  play: '<path d="M8 5l11 7-11 7z" fill="currentColor"/>',
  browse: '<path d="M12 4v11M7 10l5 5 5-5M5 20h14"/>',
  exit: '<path d="M14 5h5v14h-5M10 8l-4 4 4 4M6 12h10"/>',
  note: '<path d="M9 18V6l10-2v12"/><circle cx="6.5" cy="18" r="2.5"/><circle cx="16.5" cy="16" r="2.5"/>',
  prev: '<path d="M7 6v12M18 6l-8 6 8 6z" fill="currentColor"/>',
  next: '<path d="M17 6v12M6 6l8 6-8 6z" fill="currentColor"/>',
  pause: '<path d="M8 5v14M16 5v14"/>',
  info: '<circle cx="12" cy="12" r="8.5"/><path d="M12 11v5M12 8h.01"/>',
  bell: '<path d="M6 16V11a6 6 0 1112 0v5l2 2H4z"/><path d="M10 20a2 2 0 004 0"/>',
  // round 3
  chev: '<path d="M9 5l7 7-7 7"/>',
  down: '<path d="M6 9l6 6 6-6"/>',
  clock: '<circle cx="12" cy="12" r="8.5"/><path d="M12 7.5V12l3 2"/>',
  bpm: '<path d="M8.5 20h7l-2.2-15h-2.6z"/><path d="M12 15l5-8"/>',
  circle: '<circle cx="12" cy="12" r="7.5"/><circle cx="12" cy="12" r="2.2" fill="currentColor"/>',
  slider: '<path d="M5.5 16.5c2.5-7 10.5-9 13-3" stroke-width="5" opacity=".35"/><circle cx="5.5" cy="16.5" r="2.6" fill="currentColor"/><circle cx="18.5" cy="13.5" r="2.6"/>',
  heart: '<path d="M12 20s-7-4.4-7-10a4 4 0 017-2.6A4 4 0 0119 10c0 5.6-7 10-7 10z" fill="currentColor"/>',
  playcount: '<path d="M7 5l11 7-11 7z"/>',
  exchange: '<path d="M4 8h13l-3-3M20 16H7l3 3"/>',
  sort: '<path d="M7 4v16M3.5 16.5L7 20l3.5-3.5M17 20V4M13.5 7.5L17 4l3.5 3.5"/>',
  group: '<rect x="3.5" y="4" width="17" height="5" rx="1.5"/><rect x="3.5" y="12" width="17" height="8" rx="1.5"/>',
  filter: '<path d="M4 5h16l-6 7.5V19l-4-2v-4.5z"/>',
  globe: '<circle cx="12" cy="12" r="8.5"/><path d="M3.5 12h17M12 3.5c2.6 2.8 2.6 14.2 0 17M12 3.5c-2.6 2.8-2.6 14.2 0 17"/>',
  user: '<circle cx="12" cy="8.5" r="3.8"/><path d="M4.5 20c1-4 4-5.5 7.5-5.5s6.5 1.5 7.5 5.5"/>',
  users: '<circle cx="9" cy="9" r="3.3"/><path d="M3 19c.8-3.4 3.2-4.7 6-4.7s5.2 1.3 6 4.7"/><path d="M15.5 6a3.3 3.3 0 010 6.3M17.5 14.6c1.8.6 3 1.9 3.5 4.4"/>',
  trophy: '<path d="M8 4h8v5a4 4 0 01-8 0z"/><path d="M8 6H4.5c0 3 1.5 4.5 3.7 4.8M16 6h3.5c0 3-1.5 4.5-3.7 4.8M12 13v4M8.5 20h7"/>',
  gamepad: '<rect x="3" y="7" width="18" height="11" rx="5"/><path d="M8 10.5v4M6 12.5h4"/><circle cx="15.5" cy="11.5" r="1" fill="currentColor"/><circle cx="17.5" cy="13.5" r="1" fill="currentColor"/>',
  download: '<path d="M12 4v11M7 10l5 5 5-5M5 20h14"/>',
  stop: '<rect x="7" y="7" width="10" height="10" rx="1.5" fill="currentColor"/>',
  music: '<path d="M9 18V6l10-2v12"/><circle cx="6.5" cy="18" r="2.5"/><circle cx="16.5" cy="16" r="2.5"/>',
};
function icon(name) {
  return `<svg class="i" viewBox="0 0 24 24">${ICONS[name]}</svg>`;
}
// deterministic pseudo-random, so every render of a mockup is identical
function rng(seed) {
  let s = seed >>> 0;
  return () => ((s = (s * 1664525 + 1013904223) >>> 0) / 4294967296);
}
function variant() {
  const v = new URLSearchParams(location.search).get("v") || "refined";
  document.body.classList.add("v-" + v);
  return v;
}
// the placeholder logo: an approach ring closing on a hit circle, with the wordmark inside
function logoSVG(v, id) {
  const grad = {
    faithful: ["#7ad7ff", "#9b7bff"],
    refined: ["#ffb36b", "#ff6f9b"],
    bold: ["#33d6ff", "#ff4f8b"],
  }[v];
  return `<svg viewBox="-100 -100 200 200">
    <defs>
      <linearGradient id="lg${id}" x1="-1" y1="-1" x2="1" y2="1"><stop offset="0" stop-color="${grad[0]}"/><stop offset="1" stop-color="${grad[1]}"/></linearGradient>
      <radialGradient id="rg${id}"><stop offset="0" stop-color="#1b1030" stop-opacity=".92"/><stop offset="1" stop-color="#0d0718" stop-opacity=".92"/></radialGradient>
    </defs>
    <circle r="96" fill="none" stroke="url(#lg${id})" stroke-width="2" opacity=".55"/>
    <circle r="78" fill="url(#rg${id})"/>
    <path d="M 0 -78 A 78 78 0 1 1 -67.5 -39" fill="none" stroke="url(#lg${id})" stroke-width="13" stroke-linecap="round"/>
    <circle cx="-67.5" cy="-39" r="9" fill="#fff"/>
  </svg>`;
}
// round-2 placeholder logo: flat, one accent colour (the ring), a dark disc and the hit dot; no gradients
function flatLogo(id, word) {
  return `<svg viewBox="-100 -100 200 200" style="color:var(--accent)">
    <circle r="97" fill="none" stroke="currentColor" stroke-width="1.5" opacity=".35"/>
    <circle r="80" fill="#12131a"/>
    <path d="M 0 -80 A 80 80 0 1 1 -69.3 -40" fill="none" stroke="currentColor" stroke-width="12" stroke-linecap="butt"/>
    <circle cx="-69.3" cy="-40" r="9" fill="#fff"/>
  </svg>${word ? `<div class="word">${word}</div>` : ""}`;
}
