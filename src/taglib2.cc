#include <errno.h>
#include <string.h>

#include <v8.h>
#include <nan.h>
#include <node.h>
#include <node_buffer.h>

#include <fstream>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#define NDEBUG
#define TAGLIB_STATIC
#include <taglib/tag.h>
#include <taglib/tlist.h>
#include <taglib/fileref.h>
#include <taglib/tfile.h>
#include <taglib/flacfile.h>
#include <taglib/mp4tag.h>
#include <taglib/mp4atom.h>
#include <taglib/mp4file.h>
#include <taglib/tpicturemap.h>
#include <taglib/tpropertymap.h>
#include <taglib/tbytevector.h>
#include <taglib/tbytevectorlist.h>

using namespace std;
using namespace v8;
using namespace node;

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
  Nan::HandleScope scope;
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

  Nan::MaybeLocal<v8::String> audio_file_maybe = Nan::To<v8::String>(info[0]);
  if(audio_file_maybe.IsEmpty()) {
    Nan::ThrowTypeError("Audio file not found");
    return;
  }

  std::string audio_file = *v8::String::Utf8Value(v8::Isolate::GetCurrent(), audio_file_maybe.ToLocalChecked());

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
    v8::Maybe<bool> maybe_result = o->Has(Nan::GetCurrentContext(), Nan::New(name).ToLocalChecked());

    if(maybe_result.IsNothing()) {
      return false;
    } else {
      return maybe_result.ToChecked();
    }
  };

  auto getOptionString = [&](
      Local<v8::Object> o,
      const std::string name) -> TagLib::String {

    auto r = o->Get(Nan::GetCurrentContext(), Nan::New(name).ToLocalChecked());

    std::string st = *v8::String::Utf8Value(v8::Isolate::GetCurrent(), r.ToLocalChecked());
    return StringToTagLibString(st);
  };

  auto getOptionInt = [&](
      Local<v8::Object> o,
      const std::string name) -> int {

    auto v = o->Get(Nan::GetCurrentContext(), Nan::New(name).ToLocalChecked());

    Nan::Maybe<int> maybe = Nan::To<int32_t>(v.ToLocalChecked());

    return maybe.ToChecked();
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

  if (hasOption(options, "id")) {
    hasProps = true;
    TagLib::String value = getOptionString(options, "id");
    map.erase(TagLib::String("ID"));
    map.insert(TagLib::String("ID"), value);
  }

  if (hasOption(options, "bpm")) {
    hasProps = true;
    TagLib::String value = getOptionString(options, "bpm");
    map.erase(TagLib::String("BPM"));
    map.insert(TagLib::String("BPM"), value);
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
    auto pictures = options->Get(Nan::GetCurrentContext(), Nan::New("pictures").ToLocalChecked());
    Local<Array> pics = Local<Array>::Cast(pictures.ToLocalChecked());
    unsigned int plen = pics->Length();

    TagLib::PictureMap picMap;
    bool hasPics = false;

    for (unsigned int i = 0; i < plen; i++) {
      Local<v8::Object> imgObj = Handle<Object>::Cast(pics->Get(Nan::GetCurrentContext(), i).ToLocalChecked());

      if (!hasOption(imgObj, "mimetype")) {
        Nan::ThrowTypeError("mimetype required for each picture");
        return;
      }

      if (!hasOption(imgObj, "picture")) {
        Nan::ThrowTypeError("picture required for each item in pictures array");
        return;
      }

      auto mimetype = getOptionString(imgObj, "mimetype");
      auto picture = Nan::To<v8::Object>(
        imgObj->Get(Nan::GetCurrentContext(), Nan::New("picture").ToLocalChecked()).ToLocalChecked()
      );

      if (!picture.IsEmpty() && node::Buffer::HasInstance(picture.ToLocalChecked())) {

        char* buffer = node::Buffer::Data(picture.ToLocalChecked());
        const size_t blen = node::Buffer::Length(picture.ToLocalChecked());
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
  Nan::HandleScope scope;

  Nan::MaybeLocal<v8::String> audio_file_maybe = Nan::To<v8::String>(info[0]);
  if(audio_file_maybe.IsEmpty()) {
    Nan::ThrowTypeError("Audio file not found");
    return;
  }

  std::string audio_file = *v8::String::Utf8Value(v8::Isolate::GetCurrent(), audio_file_maybe.ToLocalChecked());

  if (!isFile(audio_file.c_str())) {
    Nan::ThrowTypeError("Audio file not found");
    return;
  }

  string ext;
  const size_t pos = audio_file.find_last_of(".");

  if (pos != -1) {
    ext = audio_file.substr(pos + 1);

    for (std::string::size_type i = 0; i < ext.length(); ++i)
      ext[i] = std::toupper(ext[i]);
  }

  TagLib::FileRef f(audio_file.c_str());
  TagLib::Tag *tag = f.tag();
  TagLib::PropertyMap map = f.properties();

  if (!tag || f.isNull()) {
    Nan::ThrowTypeError("Could not parse file");
    return;
  }

  v8::Local<v8::Object> obj = Nan::New<v8::Object>();

  if(!tag->title().isEmpty()) {
    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("title").ToLocalChecked(),
      TagLibStringToString(tag->title())
    );
  }

  if(!tag->artist().isEmpty()) {
    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("artist").ToLocalChecked(),
      TagLibStringToString(tag->artist())
    );
  }

  if (map.contains("COMPILATION")) {
    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("compilation").ToLocalChecked(),
      TagLibStringToString(map["COMPILATION"].toString(","))
    );
  }

  if (map.contains("ID")) {
    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("id").ToLocalChecked(),
      TagLibStringToString(map["ID"].toString(","))
    );
  }

  if (map.contains("ALBUMARTIST")) {
    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("albumartist").ToLocalChecked(),
      TagLibStringToString(map["ALBUMARTIST"].toString(","))
    );
  }

  if (map.contains("DISCNUMBER")) {
    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("discnumber").ToLocalChecked(),
      TagLibStringToString(map["DISCNUMBER"].toString(","))
    );
  }

  if (map.contains("TRACKNUMBER")) {
    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("tracknumber").ToLocalChecked(),
      TagLibStringToString(map["TRACKNUMBER"].toString(","))
    );
  }

  if (map.contains("BPM")) {
    TagLib::String sl = map["BPM"].toString("");
    TagLib::ByteVector vec = sl.data(TagLib::String::UTF8);
    char* s = vec.data();

    int i = 0;

    if (s) {
      i = atoi(s);
    }

    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("bpm").ToLocalChecked(),
      Nan::New<v8::Integer>(i)
    );
  }

  if (map.contains("COMPOSER")) {
    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("composer").ToLocalChecked(),
      TagLibStringToString(map["COMPOSER"].toString(","))
    );
  }

  if(!tag->album().isEmpty()) {
    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("album").ToLocalChecked(),
      TagLibStringToString(tag->album())
    );
  }

  if(!tag->comment().isEmpty()) {
    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("comment").ToLocalChecked(),
      TagLibStringToString(tag->comment())
    );
  }

  if(!tag->genre().isEmpty()) {
    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("genre").ToLocalChecked(),
      TagLibStringToString(tag->genre())
    );
  }

  auto test = obj->Set( // is always at least 0
    Nan::GetCurrentContext(),
    Nan::New("track").ToLocalChecked(),
    Nan::New<v8::Integer>(tag->track())
  );

  test = obj->Set( // is always at least 0
    Nan::GetCurrentContext(),
    Nan::New("year").ToLocalChecked(),
    Nan::New<v8::Integer>(tag->year())
  );

  // Ok, this was a quick fix. And I'll admit, opening another handle to the
  // file is not the greatest way to get the pictures for flac files. It seems
  // like this should be managed by the tag->pictures() method on FileRef, but
  // isn't, open to changes here.
  if (audio_file.find(".flac") != std::string::npos) {

    TagLib::FLAC::File flacfile(audio_file.c_str());
    TagLib::List<TagLib::FLAC::Picture *> list = flacfile.pictureList();

    size_t arraySize = list.size();
    v8::Local<v8::Array> pictures = Nan::New<v8::Array>(arraySize);

    int picIndex = 0;

    for (auto& p : list) {

      v8::Local<v8::Object> imgObj = Nan::New<v8::Object>();

      auto data = p->data();
      auto datasize = data.size();
      const char* rawdata = data.data();

      v8::Local<v8::Object> buf = Nan::NewBuffer(datasize).ToLocalChecked();
      memcpy(node::Buffer::Data(buf), rawdata, datasize);

      test = imgObj->Set(
        Nan::GetCurrentContext(),
        Nan::New("mimetype").ToLocalChecked(),
        TagLibStringToString(p->mimeType())
      );

      test = imgObj->Set(
        Nan::GetCurrentContext(),
        Nan::New("picture").ToLocalChecked(),
        buf
      );

      test = pictures->Set(
        Nan::GetCurrentContext(),
        picIndex++,
        imgObj
      );
    }

    test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("pictures").ToLocalChecked(),
      pictures
    );
  }
  else if (!tag->pictures().isEmpty()) {

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

      auto test = imgObj->Set(
        Nan::GetCurrentContext(),
        Nan::New("mimetype").ToLocalChecked(),
        TagLibStringToString(p.second[0].mime())
      );

      test = imgObj->Set(
        Nan::GetCurrentContext(),
        Nan::New("picture").ToLocalChecked(),
        buf
      );

      test = pictures->Set(
        Nan::GetCurrentContext(),
        picIndex++,
        imgObj
      );
    }

    auto test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("pictures").ToLocalChecked(),
      pictures
    );
  }

  if (f.audioProperties()) {

    TagLib::AudioProperties *properties = f.audioProperties();

    int seconds = properties->length() % 60;
    int minutes = (properties->length() - seconds) / 60;
    int hours = (properties->length() - minutes * 60) / 60;

    test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("bitrate").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->bitrate())
    );

    test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("samplerate").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->sampleRate())
    );

    test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("channels").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->channels())
    );

    // this is the same hackery, a second read, is required to get the codec
    // since codec isn't always a member of audioProperties. There should be
    // a better way of getting properties that are unique to each format.
    if (ext == "M4A" || ext == "MP4") {
      TagLib::MP4::File mp4file(audio_file.c_str());
      if (mp4file.audioProperties()) {
        auto codec = mp4file.audioProperties()->codec();

        string encoding = codec == 2 ? "alac" : "aac";

        test = obj->Set(
          Nan::GetCurrentContext(),
          Nan::New("codec").ToLocalChecked(),
          Nan::New<v8::String>(encoding).ToLocalChecked()
        );
      }
    }

    stringstream ss;
    if (hours > 0) {
      ss << hours << ":" << setfill('0') << setw(2);
    }
    ss << minutes << ":" << setfill('0') << setw(2) << seconds;
    string s = ss.str();

    auto time = Nan::New<v8::String>(s.c_str(), s.size()).ToLocalChecked();

    test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("time").ToLocalChecked(),
      time
    );

    test = obj->Set(
      Nan::GetCurrentContext(),
      Nan::New("length").ToLocalChecked(),
      Nan::New<v8::Integer>(properties->length())
    );
  }

  info.GetReturnValue().Set(obj);
}

void Init(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, void *) {
  auto test = exports->Set(
    Nan::GetCurrentContext(),
    Nan::New("writeTagsSync").ToLocalChecked(),
    Nan::New<v8::FunctionTemplate>(writeTagsSync)->GetFunction(Nan::GetCurrentContext()).ToLocalChecked()
  );

  test = exports->Set(
    Nan::GetCurrentContext(),
    Nan::New("readTagsSync").ToLocalChecked(),
    Nan::New<v8::FunctionTemplate>(readTagsSync)->GetFunction(Nan::GetCurrentContext()).ToLocalChecked()
  );
}

NODE_MODULE(taglib2, Init)
