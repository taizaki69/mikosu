// Round-3 helpers: lazer's colour language (star-rating spectrum, rank and mod colours, hue-based panel palette) and its
// outlined "triangles" texture, all drawn from scratch. Values follow ppy/osu (MIT) OsuColour/OverlayColourProvider.

const STAR_SPECTRUM = [
  [0.1, "#aaaaaa"], [0.1, "#4290fb"], [1.25, "#4fc0ff"], [2.0, "#4fffd5"], [2.5, "#7cff4f"], [3.3, "#f6f05c"],
  [4.2, "#ff8068"], [4.9, "#ff4e6f"], [5.8, "#c645b8"], [6.7, "#6563de"], [7.7, "#18158e"], [9.0, "#000000"],
];
const hexRgb = (h) => [1, 3, 5].map((i) => parseInt(h.slice(i, i + 2), 16));
const rgbHex = (c) => "#" + c.map((v) => Math.round(v).toString(16).padStart(2, "0")).join("");

function starColour(sr) {
  if (sr < STAR_SPECTRUM[0][0]) return STAR_SPECTRUM[0][1];
  for (let i = 1; i < STAR_SPECTRUM.length; i++) {
    const [p1, c1] = STAR_SPECTRUM[i];
    if (sr <= p1) {
      const [p0, c0] = STAR_SPECTRUM[i - 1];
      const t = p1 === p0 ? 1 : (sr - p0) / (p1 - p0);
      const a = hexRgb(c0), b = hexRgb(c1);
      return rgbHex(a.map((v, k) => v + (b[k] - v) * t));
    }
  }
  return "#000000";
}
const starTextColour = (sr) => (sr < 6.5 ? "rgba(0,0,0,.75)" : "#ffd966");

// grade badge colours (background, letter); S and SS letters are gradients
const RANKS = {
  SS: ["#de31ae", "linear-gradient(#ffe7a8, #ffb800)"],
  S: ["#02b5c3", "linear-gradient(#ffe7a8, #ffb800)"],
  A: ["#88da20", "#275227"],
  B: ["#e3b130", "#553a2b"],
  C: ["#ff8e5d", "#473625"],
  D: ["#ff5a5a", "#512525"],
};
// mod pill colours by mod type
const MOD_TYPE = { HD: "#ff6666", HR: "#ff6666", DT: "#ff6666", NC: "#ff6666", FL: "#ff6666", EZ: "#b2ff66", NF: "#b2ff66", HT: "#b2ff66" };

const STAR_SVG = '<svg class="st" viewBox="0 0 24 24"><path d="M12 2.6l2.9 6.1 6.6.8-4.9 4.6 1.3 6.6L12 17.4l-5.9 3.3 1.3-6.6L2.5 9.5l6.6-.8z" fill="currentColor"/></svg>';

function starPill(sr, size = "") {
  return `<span class="star ${size}" style="--c:${starColour(sr)};--t:${starTextColour(sr)}">${STAR_SVG}${sr.toFixed(2)}</span>`;
}
function rankLetter(r, cls = "") {
  const fg = RANKS[r][1];
  const style = fg.startsWith("linear") ? `background:${fg};-webkit-background-clip:text;background-clip:text;color:transparent` : `color:${fg}`;
  return `<span class="rank-letter ${cls}" style="${style}">${r}</span>`;
}
function modPill(m) {
  return `<span class="modp" style="--m:${MOD_TYPE[m] || "#8c66ff"}">${m}</span>`;
}

// lazer's TrianglesV2: thin outlined equilateral triangles of one size, scattered, fading towards the bottom
function triangles(seed, w, h, { count, size = 100, stroke = 1.6, colour = "#fff", opacity = 1 } = {}) {
  const r = rng(seed);
  const n = count ?? Math.max(1, Math.round(w * 0.02));
  const th = size * 0.866;
  let p = "";
  for (let i = 0; i < n; i++) {
    const s = size * (0.6 + r() * 0.8);
    const x = r() * w, y = r() * (h + th) - th * 0.3;
    p += `<path d="M${x.toFixed(1)} ${y.toFixed(1)}l${(s / 2).toFixed(1)} ${(s * 0.866).toFixed(1)}h${(-s).toFixed(1)}z"/>`;
  }
  const id = "tg" + seed;
  return `<svg class="tri" width="${w}" height="${h}" viewBox="0 0 ${w} ${h}" style="opacity:${opacity}">
    <defs><linearGradient id="${id}" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="0" y2="${h}"><stop offset="0" stop-color="${colour}" stop-opacity="1"/><stop offset="1" stop-color="${colour}" stop-opacity="0"/></linearGradient></defs>
    <g fill="none" stroke="url(#${id})" stroke-width="${stroke}" stroke-linejoin="round">${p}</g></svg>`;
}
