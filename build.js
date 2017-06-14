var spawn = require('win-spawn')

var ps = spawn('cmake-js', [
  'rebuild',
  '-r', process.env.npm_config_runtime || 'node',
  '-v', process.env.npm_config_target || process.versions.node,
  '--abi', process.env.npm_config_abi || process.versions.modules
])
ps.stdout.pipe(process.stdout)
ps.stderr.pipe(process.stderr)
ps.on('exit', function (code) {
  process.exit(code)
})
