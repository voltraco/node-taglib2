'use strict'
const fs = require('fs')
const taglib2 = require('../index')
const test = require('tape')
const mkdirp = require('mkdirp')
const rimraf = require('rimraf')

const TMP_PATH = __dirname + '/tmp'
const FIXTURES_PATH = __dirname + '/fixtures'

test('setup', assert => {
  try { fs.statSync(TMP_PATH) }
  catch(_) { mkdirp.sync(TMP_PATH) }
  assert.end()
})

rimraf.sync(TMP_PATH + '/*')

test('sanity', assert => {

  const rn = Math.floor(Math.random() * 100)
  const rn_year = Math.floor(Math.random() * 1000)

  const audiopath = FIXTURES_PATH + '/sample-output.mp3'
  fs.writeFileSync(audiopath, fs.readFileSync(FIXTURES_PATH + '/sample.mp3'))

  const imagepath = FIXTURES_PATH + '/sample.jpg'
  const imagefile = fs.readFileSync(imagepath)

  assert.ok(!!fs.statSync(audiopath))
  assert.ok(!!fs.statSync(imagepath))

  const r = taglib2.writeTagsSync(audiopath, {
    artist: 'artist' + rn,
    title: 'title' + rn,
    album: 'album' + rn,
    comment: 'comment' + rn,
    genre: 'genre' + rn,
    year: rn_year,
    track: '3' + rn,
    cover: imagefile 
  })

  assert.ok(r)

  const tags = taglib2.readTagsSync(audiopath)

  assert.equal(tags.artist, 'artist' + rn)
  assert.equal(tags.title, 'title' + rn)
  assert.equal(tags.album, 'album' + rn)
  assert.equal(tags.comment, 'comment' + rn)
  assert.equal(tags.genre, 'genre' + rn)
  assert.equal(tags.year, parseInt(rn_year), 10)
  assert.equal(tags.track, parseInt('3' + rn), 10)

  const tmpImagepath = TMP_PATH + '/sample.jpg'
  fs.writeFileSync(tmpImagepath, tags.pictures[0])

  assert.equal(Buffer.compare(
    fs.readFileSync(tmpImagepath),
    fs.readFileSync(imagepath)
  ), 0)

  assert.end()
})

