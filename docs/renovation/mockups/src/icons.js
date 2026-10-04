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
