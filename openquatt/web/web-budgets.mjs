// Fail when gzip growth exceeds the smaller of these two limits.
export const WEB_BUNDLE_GZIP_GROWTH_LIMIT = { bytes: 7_680, ratio: 0.03 };

export const WEB_BUNDLE_BUDGETS = [
  {
    file: "js/openquatt-app.js",
    // Includes passive learning, trend goals, the curve editor and air-purge status.
    // With daily-learning UI and legacy compatibility, plus the performance invitation.
    // Includes controlled warmup, deferred UI updates and controller-backed durations.
    // Merged with dev 46178e88: 1,330,236 B raw; gzip growth +3,260 B on Node 24.14.1.
    raw: 1_335_000,
    gzipBaselineCeiling: 362_000,
  },
  // Includes passive-learning cards and the curve editor: ~207.9 kB raw.
  { file: "css/openquatt-app.css", raw: 209_000 },
];
