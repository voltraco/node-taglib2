const binding = require('./build/Release/taglib2')
const normalize = require('path').normalize

exports.writeTagsSync = (path, options) => {
  if (typeof path !== 'string') {
    throw new TypeError('Expected a path to audio file')
  }
  path = normalize(path)
  return binding.writeTagsSync(path, options)
}

exports.readTagsSync = path => {
  if (typeof path !== 'string') {
    throw new TypeError('Expected a path to audio file')
  }
  path = normalize(path)
  return binding.readTagsSync(path)
}
