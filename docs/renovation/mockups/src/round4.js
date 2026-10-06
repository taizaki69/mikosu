// Round-4 helpers: stable's star row, the placeholder logo as a glass orb, and stable's visualiser.

const STAR = '<svg class="s" viewBox="0 0 24 24"><path d="M12 2.8l2.75 5.8 6.35.75-4.7 4.4 1.25 6.3L12 16.9l-5.65 3.15 1.25-6.3-4.7-4.4 6.35-.75z" fill="currentColor"/></svg>';

// stable shows ten stars: whole ones lit, the fraction as a smaller star, the rest dim
function stars(sr) {
  let out = "";
  for (let i = 0; i < 10; i++) {
    if (i < Math.floor(sr)) out += STAR;
    else if (i === Math.floor(sr) && sr % 1 > 0.05) {
      const f = 0.45 + 0.55 * (sr % 1);
      out += STAR.replace('class="s"', `class="s" style="transform:scale(${f.toFixed(2)})"`);
    } else out += STAR.replace('class="s"', 'class="s off"');
  }
  return `<span class="stars">${out}</span>`;
}

// the placeholder logo (not the osu! cookie), flat and matte: a frosted disc (drawn by the page as glass), a thin
// approach ring in the theme's line colours, the hit dot on the ring, and the wordmark; no gloss, no 3D
const LOGO = {
  dusk: { ring: ["#ffa8d8", "#c9b2ff", "#9fe0ff"], word: "#ffffff", edge: 0.55 },
  stable: { ring: ["#ff66aa", "#c58cff", "#66ccff"], word: "#ffffff", edge: 0.45 },
  moon: { ring: ["#a8e6ff", "#ffffff", "#a8e6ff"], word: "#f2f6ff", edge: 0.4 },
  day: { ring: ["#ff9fd0", "#c7a8ff", "#86d8ff"], word: "#2a3555", edge: 0.9 },
};
function logo(v, id, word) {
  const c = LOGO[v];
  return `<div class="disc glass"></div><svg viewBox="-110 -110 220 220">
  <defs><linearGradient id="r${id}" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="${c.ring[0]}"/><stop offset=".5" stop-color="${c.ring[1]}"/><stop offset="1" stop-color="${c.ring[2]}"/></linearGradient></defs>
  <circle r="97" fill="none" stroke="url(#r${id})" stroke-width="2.4"/>
  <circle r="84" fill="none" stroke="#fff" stroke-opacity="${c.edge}" stroke-width="1.2"/>
  <circle cx="-84" cy="-48.5" r="5.5" fill="#fff"/>
  ${word ? `<text x="0" y="11" text-anchor="middle" font-family="Outfit" font-weight="400" font-size="34" letter-spacing="1.5" fill="${c.word}">${word}</text>` : ""}
</svg>`;
}

// stable's menu visualiser: thin bars around the logo, brighter in the low frequencies
function visualiser(canvas, cx, cy, r, colour, seed) {
  const x = canvas.getContext("2d"), rand = rng(seed);
  const N = 180;
  let s = 0;
  x.lineCap = "round";
  for (let i = 0; i < N; i++) {
    s = s * 0.55 + rand() * 0.45;
    const f = i / N;
    const len = 10 + 110 * s * s * (0.45 + 0.55 * Math.pow(Math.cos(f * Math.PI * 2) * 0.5 + 0.5, 2));
    const a = f * Math.PI * 2 - Math.PI / 2;
    x.beginPath();
    x.moveTo(cx + Math.cos(a) * r, cy + Math.sin(a) * r);
    x.lineTo(cx + Math.cos(a) * (r + len), cy + Math.sin(a) * (r + len));
    x.strokeStyle = colour;
    x.lineWidth = 4;
    x.stroke();
  }
}
