const prebuildify = require('prebuildify')

const targets = [
  { runtime: 'node', target: '4.6.1' },
  { runtime: 'node', target: '5.12.0' },
  { runtime: 'node', target: '6.9.4' },
  { runtime: 'node', target: '7.4.0' },
  { runtime: 'node', target: '8.0.0' },
  { runtime: 'electron', target: '1.0.2' },
  { runtime: 'electron', target: '1.2.8' },
  { runtime: 'electron', target: '1.3.13' },
  { runtime: 'electron', target: '1.4.15' },
  { runtime: 'electron', target: '1.5.0' },
  { runtime: 'electron', target: '1.6.0' },
  { runtime: 'electron', target: '1.7.0' }
]

prebuildify({
  targets
}, err => {
  if (err) throw err
})
