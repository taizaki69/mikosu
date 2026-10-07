// Round-6 helpers: the placeholder logo (not the osu! cookie: a dark disc inside a thick graded ring, with the hit dot
// and the wordmark), and stable's menu visualiser. Uses rng() from icons.js and triangles() from round3.js.

function logo6(id, { word = "mikosu", light = false, tri = true } = {}) {
  return `<svg viewBox="-125 -125 250 250">
  <defs>
    <linearGradient id="ring${id}" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ff6fae"/><stop offset=".55" stop-color="#9a72ff"/><stop offset="1" stop-color="#66ccff"/></linearGradient>
    <radialGradient id="disc${id}" cx=".38" cy=".3" r=".9"><stop offset="0" stop-color="${light ? "#3a2c7a" : "#2c2266"}"/><stop offset="1" stop-color="${light ? "#231a52" : "#140f2e"}"/></radialGradient>
    <clipPath id="clip${id}"><circle r="93"/></clipPath>
  </defs>
  <circle r="93" fill="url(#disc${id})"/>
  ${tri ? `<g clip-path="url(#clip${id})" transform="translate(-93 -93)" opacity=".1">${triangles(91, 186, 186, { count: 9, size: 70, stroke: 2.4 }).replace(/<svg[^>]*>/, "").replace("</svg>", "")}</g>` : ""}
  <circle r="100" fill="none" stroke="url(#ring${id})" stroke-width="14"/>
  <circle cx="-86.6" cy="-50" r="8" fill="#fff"/>
  ${word ? `<text x="0" y="15" text-anchor="middle" font-family="Outfit" font-weight="600" font-size="45" letter-spacing=".5" fill="#fff">${word}</text>` : ""}
</svg>`;
}

// stable's visualiser: bars around the logo, longer in the bass
function visualiser6(canvas, cx, cy, r, colour, seed, { n = 200, width = 4, max = 120 } = {}) {
  const x = canvas.getContext("2d"), rand = rng(seed);
  let s = 0;
  x.lineCap = "round";
  for (let i = 0; i < n; i++) {
    s = s * 0.55 + rand() * 0.45;
    const f = i / n;
    const len = 8 + max * s * s * (0.4 + 0.6 * Math.pow(Math.cos(f * Math.PI * 2) * 0.5 + 0.5, 2));
    const a = f * Math.PI * 2 - Math.PI / 2;
    x.beginPath();
    x.moveTo(cx + Math.cos(a) * r, cy + Math.sin(a) * r);
    x.lineTo(cx + Math.cos(a) * (r + len), cy + Math.sin(a) * (r + len));
    x.strokeStyle = colour;
    x.lineWidth = width;
    x.stroke();
  }
}
