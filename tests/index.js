'use strict'
const fs = require('fs')
const taglib2 = require('../index')
const test = require('tape')
const mkdirp = require('mkdirp')
const path = require('path')
const rimraf = require('rimraf')
const http = require('http')

const TMP_PATH = path.join(__dirname, '/tmp')
const FIXTURES_PATH = path.join(__dirname, '/fixtures')

;(function setup () {
  try {
    fs.statSync(TMP_PATH)
  } catch (_) {
    mkdirp.sync(TMP_PATH)
  }

  rimraf.sync(TMP_PATH + '/*')

  const flacfilesource = [
    'http://www.eclassical.com',
    '/custom/eclassical/files/BIS1447-002-flac_24.flac'
  ].join('')

  const flacfile = FIXTURES_PATH + '/classical å æ ø ö ä ù ó ð.flac'

  fs.stat(flacfile, (err, stat) => {
    if (!err) return onReady()

    const ws = fs.createWriteStream(flacfile)
    http.get(flacfilesource, res => {
      res.pipe(ws)
      ws.on('end', onReady)
    })
  })
})()

function onReady () {
  test('extract images from flac', assert => {
    // extrat the images in the flac file (this file we know has 6 images)
    const p = FIXTURES_PATH + '/classical å æ ø ö ä ù ó ð.flac'
    const ps = fs.statSync(p)
    assert.ok(ps.size > 0, 'flac file downloaded properly')

    const tags = taglib2.readTagsSync(p)
    assert.equal(tags.pictures.length, 6)

    // write the first one to the tmp directory
    const mime = tags.pictures[0].mimetype.split('/')
    const writepath = TMP_PATH + '/classical.' + mime[1]
    fs.writeFileSync(writepath, tags.pictures[0].picture)

    // compare the extracted one to the original image
    const extractedImage = fs.readFileSync(writepath)
    const originalImage = fs.readFileSync(FIXTURES_PATH + '/classical.jpeg')
    assert.equal(extractedImage.length, originalImage.length)
    assert.end()
  })

  test('sync write/read', assert => {
    function getRandomYear () {
      const date = new Date()
      const from = date.setFullYear(1877, 0, 1)
      const to = (new Date()).getTime()
      return new Date(from + Math.random() * (to - from)).getFullYear()
    }

    const rn = Math.floor(Math.random() * 100)
    const rn_year = getRandomYear()

    const audiopath = FIXTURES_PATH + '/sample-output å æ ø ö ä ù ó ð.mp3'
    fs.writeFileSync(audiopath, fs.readFileSync(FIXTURES_PATH + '/sample.mp3'))

    const imagepath = FIXTURES_PATH + '/sample.jpg'
    const imagefile = fs.readFileSync(imagepath)

    assert.ok(!!fs.statSync(audiopath))
    assert.ok(!!fs.statSync(imagepath))

    assert.throws(() => {
      taglib2.writeTagsSync()
    }, 'not enough arguments')

    const r = taglib2.writeTagsSync(audiopath, {
      artist: 'å æ ø ö ä ù ó ð ärtist' + rn,
      albumartist: 'å æ ø ö ä ù ó ð albumartist' + rn,
      title: 'å æ ø ö ä ù ó ð title' + rn,
      album: 'å æ ø ö ä ù ó ð album' + rn,
      comment: 'å æ ø ö ä ù ó ð comment' + rn,
      genre: 'å æ ø ö ä ù ó ð genre' + rn,
      year: rn_year,
      // track: 3 + rn,
      tracknumber: '3/' + rn,
      discnumber: '1/' + rn,
      id: rn,
      composer: 'å æ ø ö ä ù ó ð composer' + rn,
      bpm: parseInt(rn, 10),
      pictures: [{ mimetype: '', picture: imagefile }]
    })

    assert.ok(r)

    assert.throws(() => {
      taglib2.readTagsSync()
    }, 'not enough arguments')

    const tags = taglib2.readTagsSync(audiopath)

    assert.equal(tags.artist, 'å æ ø ö ä ù ó ð ärtist' + rn)
    assert.equal(tags.albumartist, 'å æ ø ö ä ù ó ð albumartist' + rn)
    assert.equal(tags.title, 'å æ ø ö ä ù ó ð title' + rn)
    assert.equal(tags.bpm, rn)
    assert.equal(tags.album, 'å æ ø ö ä ù ó ð album' + rn)
    assert.equal(tags.comment, 'å æ ø ö ä ù ó ð comment' + rn)
    assert.equal(tags.genre, 'å æ ø ö ä ù ó ð genre' + rn)
    assert.equal(tags.year, parseInt(rn_year, 10))
    assert.equal(tags.discnumber, '1/' + rn)
    assert.equal(tags.id, String(rn))
    assert.equal(tags.composer, 'å æ ø ö ä ù ó ð composer' + rn)
    // assert.equal(tags.track, 3 + rn)
    assert.equal(tags.tracknumber, '3/' + rn)

    const tmpImagepath = TMP_PATH + '/sample.jpg'
    fs.writeFileSync(tmpImagepath, tags.pictures[0].picture)

    assert.equal(tags.bitrate, 192)
    assert.equal(tags.samplerate, 44100)
    // Lossy formats don't have bits per sample
    assert.equal(tags.bitsPerSample, null)
    assert.equal(tags.channels, 2)
    assert.equal(tags.length, 90)
    assert.equal(tags.time, '1:30')

    assert.equal(Buffer.compare(
      fs.readFileSync(tmpImagepath),
      fs.readFileSync(imagepath)
    ), 0)

    assert.end()
  })

  test('sync write/read m4a + jpg', assert => {
    const audiopath = FIXTURES_PATH + '/sample-output å æ ø ö ä ù ó ð.m4a'
    fs.writeFileSync(audiopath, fs.readFileSync(FIXTURES_PATH + '/sample.m4a'))

    const imagepath = FIXTURES_PATH + '/sample.jpg'
    const imagefile = fs.readFileSync(imagepath)

    assert.comment('write a copy of an mp4 file with a new image')

    taglib2.writeTagsSync(audiopath, {
      pictures: [{
        mimetype: 'image/jpeg',
        picture: imagefile
      }]
    })

    assert.comment('read the tags from the new file')
    const tags = taglib2.readTagsSync(audiopath)

    assert.comment('write the picture to a tmp file')
    const tmpImagepath = TMP_PATH + '/sample.jpg'
    fs.writeFileSync(tmpImagepath, tags.pictures[0].picture)

    assert.comment('compare the image extracted to the image added')

    assert.equal(Buffer.compare(
      fs.readFileSync(tmpImagepath),
      fs.readFileSync(imagepath)
    ), 0)

    assert.end()
  })

  test('sync write/read m4a + png', assert => {
    const audiopath = FIXTURES_PATH + '/sample-output-png.m4a'
    fs.writeFileSync(audiopath, fs.readFileSync(FIXTURES_PATH + '/sample.m4a'))

    const imagepath = FIXTURES_PATH + '/sample.png'
    const imagefile = fs.readFileSync(imagepath)

    assert.comment('write a copy of an mp4 file with a new image')

    taglib2.writeTagsSync(audiopath, {
      pictures: [{
        mimetype: 'image/png',
        picture: imagefile
      }]
    })

    assert.comment('read the tags from the new file')
    const tags = taglib2.readTagsSync(audiopath)

    assert.comment('write the picture to a tmp file')
    const tmpImagepath = TMP_PATH + '/sample.png'
    fs.writeFileSync(tmpImagepath, tags.pictures[0].picture)

    assert.comment('compare the image extracted to the image added')

    assert.equal(Buffer.compare(
      fs.readFileSync(tmpImagepath),
      fs.readFileSync(imagepath)
    ), 0)

    assert.end()
  })

  test('sync read bitsPerSample FLAC', assert => {
    const audiopath = FIXTURES_PATH + '/sample.flac'
    const tags = taglib2.readTagsSync(audiopath)

    assert.notEqual(tags.bitsPerSample, 24)
    assert.end()
  })

  test('sync read bitsPerSample WAV', assert => {
    const audiopath = FIXTURES_PATH + '/sample.wav'
    const tags = taglib2.readTagsSync(audiopath)

    assert.equal(tags.bitsPerSample, 16)
    assert.end()
  })

  test('sync read bitsPerSample AIFF', assert => {
    const audiopath = FIXTURES_PATH + '/sample.aifc'
    const tags = taglib2.readTagsSync(audiopath)

    assert.equal(tags.bitsPerSample, 16)
    assert.end()
  })
}
