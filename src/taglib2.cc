#include <errno.h>
#include <string.h>

#include <v8.h>
#include <nan.h>
#include <node_buffer.h>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <tag.h>
#include <tlist.h>
#include <fileref.h>
#include <tfile.h>
#include <tpicturemap.h>
#include <tpropertymap.h>
#include <tbytevector.h>
#include <tbytevectorlist.h>

using namespace std;
using namespace v8;

Local<Value> TagLibStringToString(TagLib::String s) {

  if (s.isEmpty()) return Nan::Null();

  TagLib::ByteVector str = s.data(TagLib::String::UTF16);

  return Nan::New<v8::String>(
    (uint16_t *) str.mid(2, str.size()-2).data(),
    s.size()
  ).ToLocalChecked();
}

TagLib::String StringToTagLibString(std::string s) {
  return TagLib::String(s, TagLib::String::UTF8);
}

bool isFile(const char *s) {
  struct stat st;
#ifdef _WIN32
  return ::stat(s, &st) == 0 && (st.st_mode & (S_IFREG));
#else
  return ::stat(s, &st) == 0 && (st.st_mode & (S_IFREG | S_IFLNK));
#endif
}

NAN_METHOD(writeTagsSync) {
  Local<v8::Object> options;

  if (info.Length() < 2) {
    Nan::ThrowTypeError("Expected 2 arguments");
    return;
  }

  if (!info[0]->IsString()) {
    Nan::ThrowTypeError("Expected a path to audio file");
    return;
  }

  if (!info[1]->IsObject()) return;

  options = v8::Local<v8::Object>::Cast(info[1]);
  std::string audio_file = *v8::String::Utf8Value(info[0]->ToString());

  if (!isFile(audio_file.c_str())) {
    Nan::ThrowTypeError("Audio file not found");
    return;
  }

  TagLib::FileRef f(audio_file.c_str());
  TagLib::Tag *tag = f.tag();
  TagLib::PropertyMap map = f.properties();

  if (!tag) {
    Nan::ThrowTypeError("Could not parse file");
    return;
  }

  bool hasProps = false;

  auto hasOption = [&](
    Local<v8::Object> o,
    const std::string name) -> bool {

    return o->Has(Nan::New(name).ToLocalChecked());
  };

  auto getOptionString = [&](
      Local<v8::Object> o,
      const std::string name) -> TagLib::String {

    auto r = o->Get(Nan::New(name).ToLocalChecked());
    std::string s = *v8::String::Utf8Value(r);
    return StringToTagLibString(s);
  };

  auto getOptionInt = [&](
      Local<v8::Object> o,
      const std::string name) -> int {

    return o->Get(Nan::New(name).ToLocalChecked())->Int32Value();
  };

  if (hasOption(options, "albumartist")) {
    hasProps = true;
    TagLib::String value = getOptionString(options, "albumartist");
    map.erase(TagLib::String("ALBUMARTIST"));
    map.insert(TagLib::String("ALBUMARTIST"), value);
  }

  if (hasOption(options, "discnumber")) {
    hasProps = true;
    TagLib::String value = getOptionString(options, "discnumber");
    map.erase(TagLib::String("DISCNUMBER"));
    map.insert(TagLib::String("DISCNUMBER"), value);
  }

  if (hasOption(options, "tracknumber")) {
    hasProps = true;
    TagLib::String value = getOptionString(options, "tracknumber");
    map.erase(TagLib::String("TRACKNUMBER"));
    map.insert(TagLib::String("TRACKNUMBER"), value);
  }

  if (hasOption(options, "composer")) {
    hasProps = true;
    TagLib::String value = getOptionString(options, "composer");
    map.erase(TagLib::String("COMPOSER"));
    map.insert(TagLib::String("COMPOSER"), value);
  }

  if (hasProps) {
    f.setProperties(map);
  }

  if (hasOption(options, "artist")) {
    tag->setArtist(getOptionString(options, "artist"));
  }

  if (hasOption(options, "title")) {
    tag->setTitle(getOptionString(options, "title"));
  }

  if (hasOption(options, "album")) {
    tag->setAlbum(getOptionString(options, "album"));
  }

  if (hasOption(options, "comment")) {
    tag->setComment(getOptionString(options, "comment"));
  }

  if (hasOption(options, "genre")) {
    tag->setGenre(getOptionString(options, "genre"));
  }

  if (hasOption(options, "year")) {
    tag->setYear(getOptionInt(options, "year"));
  }

  if (hasOption(options, "track")) {
    tag->setTrack(getOptionInt(options, "track"));
  }

  if (hasOption(options, "pictures")) {
    auto pictures = options->Get(Nan::New("pictures").ToLocalChecked());
    Local<Array> pics = Local<Array>::Cast(pictures);
    unsigned int plen = pics->Length();

    TagLib::PictureMap picMap;
    bool hasPics = false;

    for (unsigned int i = 0; i < plen; i++) {
      Local<v8::Object> imgObj = Handle<Object>::Cast(pics->Get(i));

      if (!hasOption(imgObj, "mimetype")) {
        Nan::ThrowTypeError("mimetype required for each picture");
        return;
      }

      if (!hasOption(imgObj, "picture")) {
        Nan::ThrowTypeError("picture required for each item in pictures array");
        return;
      }

      auto mimetype = getOptionString(imgObj, "mimetype");
      auto picture = imgObj->Get(Nan::New("picture").ToLocalChecked());

      if (!picture.IsEmpty() && node::Buffer::HasInstance(picture->ToObject())) {

        char* buffer = node::Buffer::Data(picture->ToObject());
        const size_t blen = node::Buffer::Length(picture->ToObject());
        TagLib::ByteVector data(buffer, blen);

        TagLib::Picture pic(data,
          TagLib::Picture::FrontCover,
          mimetype,
          "Added with node-taglib2");

        picMap.insert(pic);
        hasPics = true;
      }
    }

    if (hasPics) {
      tag->setPictures(picMap);
    }
  }

  f.save();

  info.GetReturnValue().Set(Nan::True());
}

NAN_METHOD(readTagsSync) {

  if (!info[0]->IsString()) {
    Nan::ThrowTypeError("Expected a path to audio file");
    return;
  }

  std::string audio_file = *v8::String::Utf8Value(info[0]->ToString());

  if (!isFile(audio_file.c_str())) {
    Nan::ThrowTypeError("Audio file not found");
    return;
  }

  TagLib::FileRef f(audio_file.c_str());
  TagLib::Tag *tag = f.tag();
  TagLib::PropertyMap map = f.properties();

  if (!tag || f.isNull()) {
    Nan::ThrowTypeError("Could not parse file");
    return;
  }

  v8::Local<v8::Object> obj = Nan::New<v8::Object>();

  !tag->title().isEmpty() && obj->Set(
    Nan::New("title").ToLocalChecked(),
    TagLibStringToString(tag->title())
  );

  !tag->artist().isEmpty() && obj->Set(
    Nan::New("artist").ToLocalChecked(),
    TagLibStringToString(tag->artist())
  );

  if (map.contains("ALBUMARTIST")) {
    obj->Set(
      Nan::New("albumartist").ToLocalChecked(),
      TagLibStringToString(map["ALBUMARTIST"].toString(","))
    );
  }

  if (map.contains("DISCNUMBER")) {
    obj->Set(
      Nan::New("discnumber").ToLocalChecked(),
      TagLibStringToString(map["DISCNUMBER"].toString(","))
    );
  }

  if (map.contains("TRACKNUMBER")) {
    obj->Set(
      Nan::New("tracknumber").ToLocalChecked(),
      TagLibStringToString(map["TRACKNUMBER"].toString(","))
    );
  }

  if (map.contains("BPM")) {
    TagLib::String sl = map["BPM"].toString("");
    TagLib::ByteVector vec = sl.data(TagLib::String::Latin1);
    char* s = vec.data();

    obj->Set(
      Nan::New("bpm").ToLocalChecked(),
      Nan::New<v8::Integer>(atoi(s))
    );
  }

  if (map.contains("COMPOSER")) {
    obj->Set(
      Nan::New("composer").ToLocalChecked(),
      TagLibStringToString(map["COMPOSER"].toString(","))
    );
  }

  !tag->album().isEmpty() && obj->Set(
    Nan::New("album").ToLocalChecked(),
    TagLibStringToString(tag->album())
  );

  !tag->comment().isEmpty() && obj->Set(
    Nan::New("comment").ToLocalChecked(),
    TagLibStringToString(tag->comment())
  );

  !tag->genre().isEmpty() && obj->Set(
    Nan::New("genre").ToLocalChecked(),
    TagLibStringToString(tag->genre())
  );

  obj->Set( // is always at least 0
    Nan::New("track").ToLocalChecked(),
    Nan::New<v8::Integer>(tag->track())
  );

  obj->Set( // is always at least 0
    Nan::New("year").ToLocalChecked(),
    Nan::New<v8::Integer>(tag->year())
  );

  if (!tag->pictures().isEmpty()) {

    size_t arraySize = tag->pictures().size();
    v8::Local<v8::Array> pictures = Nan::New<v8::Array>(arraySize);
    int picIndex = 0;

    for (auto& p : tag->pictures()) {
  
      v8::Local<v8::Object> imgObj = Nan::New<v8::Object>();

      auto data = p.second[0].data();
      auto datasize = data.size();
      const char* rawdata = data.data();

      v8::Local<v8::Object> buf = Nan::NewBuffer(datasize).ToLocalChecked();
      memcpy(node::Buffer::Data(buf), rawdata, datasize);

      imgObj->Set(
        Nan::New("mimetype").ToLocalChecked(),
        TagLibStringToString(p.second[0].mime())
      );

      imgObj->Set(
        Nan::New("picture").ToLocalChecked(),
        buf
      );

      pictures->Set(picIndex++, imgObj);
    }

    obj->Set(Nan::New("pictures").ToLocalChecked(), pictures);
  }

  if(f.audioProperties()) {

    TagLib::AudioProperties *properties = f.audioProperties();

    int seconds = properties->length() % 60;
    int minutes = (properties->length() - seconds) / 60;

    obj->Set(
      Nan::New("bitrate").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->bitrate())
    );

    obj->Set(
      Nan::New("samplerate").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->sampleRate())
    );

    obj->Set(
      Nan::New("channels").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->channels())
    );

    stringstream ss;
    ss << minutes << ":" << setfill('0') << setw(2) << seconds;
    string s = ss.str();

    auto time = Nan::New<v8::String>(s.c_str(), s.size()).ToLocalChecked();

    obj->Set(Nan::New("time").ToLocalChecked(), time);

    obj->Set(
      Nan::New("length").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->length())
    );
  }

  info.GetReturnValue().Set(obj);
}

void Init(v8::Local<v8::Object> exports) {
  exports->Set(Nan::New("writeTagsSync").ToLocalChecked(),
    Nan::New<v8::FunctionTemplate>(writeTagsSync)->GetFunction());

  exports->Set(Nan::New("readTagsSync").ToLocalChecked(),
    Nan::New<v8::FunctionTemplate>(readTagsSync)->GetFunction());
}

NODE_MODULE(taglib2, Init)

