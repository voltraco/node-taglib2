var spawn = require('win-spawn')

var runtime = process.env.npm_config_runtime || 'node'
var target = process.env.npm_config_target || process.versions.node
var abi = process.env.npm_config_abi || process.versions.modules

console.log('BUILD for %s@%s (abi=%s)', runtime, target, abi)

var ps = spawn('cmake-js', [
  'rebuild',
  '-r', runtime,
  '-v', target,
  '--abi', abi
])
ps.stdout.pipe(process.stdout)
ps.stderr.pipe(process.stderr)
ps.on('exit', function (code) {
  process.exit(code)
})
