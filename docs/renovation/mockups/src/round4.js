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

const ORB = {
  stable: { disc: ["#ffa6d2", "#e86ad0", "#7a63ff"], ring: ["#ff66aa", "#c58cff", "#66ccff"], glow: "#ff6fb4", word: "#ffffff", rim: 0.4 },
  moon: { disc: ["#3a5490", "#1c2a52", "#0e1734"], ring: ["#a8e6ff", "#ffffff", "#a8e6ff"], glow: "#7fd0ff", word: "#f2f6ff", rim: 0.3 },
  day: { disc: ["#ffffff", "#fff0f7", "#ffd2e8"], ring: ["#ff9fd0", "#c7a8ff", "#86d8ff"], glow: "#ffb0d8", word: "#ff6fb4", rim: 0.9 },
};

// placeholder logo (not the osu! cookie): a glass orb inside an approach ring, with the hit dot on the ring
function orb(v, id, word) {
  const c = ORB[v];
  return `<svg viewBox="-110 -110 220 220">
  <defs>
    <radialGradient id="d${id}" cx="35%" cy="28%" r="80%"><stop offset="0" stop-color="${c.disc[0]}"/><stop offset=".55" stop-color="${c.disc[1]}"/><stop offset="1" stop-color="${c.disc[2]}"/></radialGradient>
    <linearGradient id="r${id}" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="${c.ring[0]}"/><stop offset=".5" stop-color="${c.ring[1]}"/><stop offset="1" stop-color="${c.ring[2]}"/></linearGradient>
    <radialGradient id="g${id}"><stop offset=".62" stop-color="${c.glow}" stop-opacity=".55"/><stop offset="1" stop-color="${c.glow}" stop-opacity="0"/></radialGradient>
    <linearGradient id="s${id}" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#fff" stop-opacity=".62"/><stop offset="1" stop-color="#fff" stop-opacity="0"/></linearGradient>
    <filter id="f${id}" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="3"/></filter>
  </defs>
  <circle r="110" fill="url(#g${id})"/>
  <circle r="97" fill="none" stroke="url(#r${id})" stroke-width="3.5" opacity=".95"/>
  <circle r="97" fill="none" stroke="url(#r${id})" stroke-width="7" opacity=".35" filter="url(#f${id})"/>
  <circle r="83" fill="url(#d${id})"/>
  <circle r="82.2" fill="none" stroke="#fff" stroke-opacity="${c.rim}" stroke-width="1.6"/>
  <ellipse cx="-14" cy="-44" rx="54" ry="27" fill="url(#s${id})"/>
  <circle cx="-84" cy="-48.5" r="11" fill="#fff" opacity=".45" filter="url(#f${id})"/>
  <circle cx="-84" cy="-48.5" r="6.5" fill="#fff"/>
  ${word ? `<text x="0" y="13" text-anchor="middle" font-family="Outfit" font-weight="600" font-size="40" letter-spacing="-.5" fill="${c.word}">${word}</text>` : ""}
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
