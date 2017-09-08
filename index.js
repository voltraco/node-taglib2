const binding = require('./build/Release/taglib2')
const fs = require('fs')

exports.writeTagsSync = (path, options) => {
  if (typeof path !== 'string') {
    throw new TypeError('Expected a path to audio file')
  }
  try {
    fs.statSync(path)
  } catch (err) {
    throw new Error('Audio file not found: ' + path)
  }
  return binding.writeTagsSync(path, options)
}

exports.readTagsSync = path => {
  if (typeof path !== 'string') {
    throw new TypeError('Expected a path to audio file')
  }
  try {
    fs.statSync(path)
  } catch (err) {
    throw new Error('Audio file not found: ' + path)
  }
  return binding.readTagsSync(path)
}
