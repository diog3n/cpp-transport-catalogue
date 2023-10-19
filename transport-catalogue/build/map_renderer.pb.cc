// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: map_renderer.proto

#include "map_renderer.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace serialize_transport_catalogue {
PROTOBUF_CONSTEXPR RenderSettings::RenderSettings(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.color_palette_)*/{}
  , /*decltype(_impl_.bus_label_offset_)*/nullptr
  , /*decltype(_impl_.stop_label_offset_)*/nullptr
  , /*decltype(_impl_.underlayer_color_)*/nullptr
  , /*decltype(_impl_.width_)*/0
  , /*decltype(_impl_.height_)*/0
  , /*decltype(_impl_.padding_)*/0
  , /*decltype(_impl_.line_width_)*/0
  , /*decltype(_impl_.stop_radius_)*/0
  , /*decltype(_impl_.bus_label_font_size_)*/0
  , /*decltype(_impl_.stop_label_font_size_)*/0
  , /*decltype(_impl_.underlayer_width_)*/0
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct RenderSettingsDefaultTypeInternal {
  PROTOBUF_CONSTEXPR RenderSettingsDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~RenderSettingsDefaultTypeInternal() {}
  union {
    RenderSettings _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 RenderSettingsDefaultTypeInternal _RenderSettings_default_instance_;
}  // namespace serialize_transport_catalogue
static ::_pb::Metadata file_level_metadata_map_5frenderer_2eproto[1];
static constexpr ::_pb::EnumDescriptor const** file_level_enum_descriptors_map_5frenderer_2eproto = nullptr;
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_map_5frenderer_2eproto = nullptr;

const uint32_t TableStruct_map_5frenderer_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.width_),
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.height_),
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.padding_),
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.line_width_),
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.stop_radius_),
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.bus_label_font_size_),
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.bus_label_offset_),
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.stop_label_font_size_),
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.stop_label_offset_),
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.underlayer_color_),
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.underlayer_width_),
  PROTOBUF_FIELD_OFFSET(::serialize_transport_catalogue::RenderSettings, _impl_.color_palette_),
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::serialize_transport_catalogue::RenderSettings)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::serialize_transport_catalogue::_RenderSettings_default_instance_._instance,
};

const char descriptor_table_protodef_map_5frenderer_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\022map_renderer.proto\022\035serialize_transpor"
  "t_catalogue\032\tsvg.proto\"\274\003\n\016RenderSetting"
  "s\022\r\n\005width\030\001 \001(\001\022\016\n\006height\030\002 \001(\001\022\017\n\007padd"
  "ing\030\003 \001(\001\022\022\n\nline_width\030\004 \001(\001\022\023\n\013stop_ra"
  "dius\030\005 \001(\001\022\033\n\023bus_label_font_size\030\006 \001(\001\022"
  ">\n\020bus_label_offset\030\007 \001(\0132$.serialize_tr"
  "ansport_catalogue.Point\022\034\n\024stop_label_fo"
  "nt_size\030\010 \001(\001\022\?\n\021stop_label_offset\030\t \001(\013"
  "2$.serialize_transport_catalogue.Point\022>"
  "\n\020underlayer_color\030\n \001(\0132$.serialize_tra"
  "nsport_catalogue.Color\022\030\n\020underlayer_wid"
  "th\030\013 \001(\001\022;\n\rcolor_palette\030\014 \003(\0132$.serial"
  "ize_transport_catalogue.Colorb\006proto3"
  ;
static const ::_pbi::DescriptorTable* const descriptor_table_map_5frenderer_2eproto_deps[1] = {
  &::descriptor_table_svg_2eproto,
};
static ::_pbi::once_flag descriptor_table_map_5frenderer_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_map_5frenderer_2eproto = {
    false, false, 517, descriptor_table_protodef_map_5frenderer_2eproto,
    "map_renderer.proto",
    &descriptor_table_map_5frenderer_2eproto_once, descriptor_table_map_5frenderer_2eproto_deps, 1, 1,
    schemas, file_default_instances, TableStruct_map_5frenderer_2eproto::offsets,
    file_level_metadata_map_5frenderer_2eproto, file_level_enum_descriptors_map_5frenderer_2eproto,
    file_level_service_descriptors_map_5frenderer_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_map_5frenderer_2eproto_getter() {
  return &descriptor_table_map_5frenderer_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_map_5frenderer_2eproto(&descriptor_table_map_5frenderer_2eproto);
namespace serialize_transport_catalogue {

// ===================================================================

class RenderSettings::_Internal {
 public:
  static const ::serialize_transport_catalogue::Point& bus_label_offset(const RenderSettings* msg);
  static const ::serialize_transport_catalogue::Point& stop_label_offset(const RenderSettings* msg);
  static const ::serialize_transport_catalogue::Color& underlayer_color(const RenderSettings* msg);
};

const ::serialize_transport_catalogue::Point&
RenderSettings::_Internal::bus_label_offset(const RenderSettings* msg) {
  return *msg->_impl_.bus_label_offset_;
}
const ::serialize_transport_catalogue::Point&
RenderSettings::_Internal::stop_label_offset(const RenderSettings* msg) {
  return *msg->_impl_.stop_label_offset_;
}
const ::serialize_transport_catalogue::Color&
RenderSettings::_Internal::underlayer_color(const RenderSettings* msg) {
  return *msg->_impl_.underlayer_color_;
}
void RenderSettings::clear_bus_label_offset() {
  if (GetArenaForAllocation() == nullptr && _impl_.bus_label_offset_ != nullptr) {
    delete _impl_.bus_label_offset_;
  }
  _impl_.bus_label_offset_ = nullptr;
}
void RenderSettings::clear_stop_label_offset() {
  if (GetArenaForAllocation() == nullptr && _impl_.stop_label_offset_ != nullptr) {
    delete _impl_.stop_label_offset_;
  }
  _impl_.stop_label_offset_ = nullptr;
}
void RenderSettings::clear_underlayer_color() {
  if (GetArenaForAllocation() == nullptr && _impl_.underlayer_color_ != nullptr) {
    delete _impl_.underlayer_color_;
  }
  _impl_.underlayer_color_ = nullptr;
}
void RenderSettings::clear_color_palette() {
  _impl_.color_palette_.Clear();
}
RenderSettings::RenderSettings(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:serialize_transport_catalogue.RenderSettings)
}
RenderSettings::RenderSettings(const RenderSettings& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  RenderSettings* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.color_palette_){from._impl_.color_palette_}
    , decltype(_impl_.bus_label_offset_){nullptr}
    , decltype(_impl_.stop_label_offset_){nullptr}
    , decltype(_impl_.underlayer_color_){nullptr}
    , decltype(_impl_.width_){}
    , decltype(_impl_.height_){}
    , decltype(_impl_.padding_){}
    , decltype(_impl_.line_width_){}
    , decltype(_impl_.stop_radius_){}
    , decltype(_impl_.bus_label_font_size_){}
    , decltype(_impl_.stop_label_font_size_){}
    , decltype(_impl_.underlayer_width_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  if (from._internal_has_bus_label_offset()) {
    _this->_impl_.bus_label_offset_ = new ::serialize_transport_catalogue::Point(*from._impl_.bus_label_offset_);
  }
  if (from._internal_has_stop_label_offset()) {
    _this->_impl_.stop_label_offset_ = new ::serialize_transport_catalogue::Point(*from._impl_.stop_label_offset_);
  }
  if (from._internal_has_underlayer_color()) {
    _this->_impl_.underlayer_color_ = new ::serialize_transport_catalogue::Color(*from._impl_.underlayer_color_);
  }
  ::memcpy(&_impl_.width_, &from._impl_.width_,
    static_cast<size_t>(reinterpret_cast<char*>(&_impl_.underlayer_width_) -
    reinterpret_cast<char*>(&_impl_.width_)) + sizeof(_impl_.underlayer_width_));
  // @@protoc_insertion_point(copy_constructor:serialize_transport_catalogue.RenderSettings)
}

inline void RenderSettings::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.color_palette_){arena}
    , decltype(_impl_.bus_label_offset_){nullptr}
    , decltype(_impl_.stop_label_offset_){nullptr}
    , decltype(_impl_.underlayer_color_){nullptr}
    , decltype(_impl_.width_){0}
    , decltype(_impl_.height_){0}
    , decltype(_impl_.padding_){0}
    , decltype(_impl_.line_width_){0}
    , decltype(_impl_.stop_radius_){0}
    , decltype(_impl_.bus_label_font_size_){0}
    , decltype(_impl_.stop_label_font_size_){0}
    , decltype(_impl_.underlayer_width_){0}
    , /*decltype(_impl_._cached_size_)*/{}
  };
}

RenderSettings::~RenderSettings() {
  // @@protoc_insertion_point(destructor:serialize_transport_catalogue.RenderSettings)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void RenderSettings::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  _impl_.color_palette_.~RepeatedPtrField();
  if (this != internal_default_instance()) delete _impl_.bus_label_offset_;
  if (this != internal_default_instance()) delete _impl_.stop_label_offset_;
  if (this != internal_default_instance()) delete _impl_.underlayer_color_;
}

void RenderSettings::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void RenderSettings::Clear() {
// @@protoc_insertion_point(message_clear_start:serialize_transport_catalogue.RenderSettings)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.color_palette_.Clear();
  if (GetArenaForAllocation() == nullptr && _impl_.bus_label_offset_ != nullptr) {
    delete _impl_.bus_label_offset_;
  }
  _impl_.bus_label_offset_ = nullptr;
  if (GetArenaForAllocation() == nullptr && _impl_.stop_label_offset_ != nullptr) {
    delete _impl_.stop_label_offset_;
  }
  _impl_.stop_label_offset_ = nullptr;
  if (GetArenaForAllocation() == nullptr && _impl_.underlayer_color_ != nullptr) {
    delete _impl_.underlayer_color_;
  }
  _impl_.underlayer_color_ = nullptr;
  ::memset(&_impl_.width_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&_impl_.underlayer_width_) -
      reinterpret_cast<char*>(&_impl_.width_)) + sizeof(_impl_.underlayer_width_));
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* RenderSettings::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // double width = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 9)) {
          _impl_.width_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<double>(ptr);
          ptr += sizeof(double);
        } else
          goto handle_unusual;
        continue;
      // double height = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 17)) {
          _impl_.height_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<double>(ptr);
          ptr += sizeof(double);
        } else
          goto handle_unusual;
        continue;
      // double padding = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 25)) {
          _impl_.padding_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<double>(ptr);
          ptr += sizeof(double);
        } else
          goto handle_unusual;
        continue;
      // double line_width = 4;
      case 4:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 33)) {
          _impl_.line_width_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<double>(ptr);
          ptr += sizeof(double);
        } else
          goto handle_unusual;
        continue;
      // double stop_radius = 5;
      case 5:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 41)) {
          _impl_.stop_radius_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<double>(ptr);
          ptr += sizeof(double);
        } else
          goto handle_unusual;
        continue;
      // double bus_label_font_size = 6;
      case 6:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 49)) {
          _impl_.bus_label_font_size_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<double>(ptr);
          ptr += sizeof(double);
        } else
          goto handle_unusual;
        continue;
      // .serialize_transport_catalogue.Point bus_label_offset = 7;
      case 7:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 58)) {
          ptr = ctx->ParseMessage(_internal_mutable_bus_label_offset(), ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // double stop_label_font_size = 8;
      case 8:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 65)) {
          _impl_.stop_label_font_size_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<double>(ptr);
          ptr += sizeof(double);
        } else
          goto handle_unusual;
        continue;
      // .serialize_transport_catalogue.Point stop_label_offset = 9;
      case 9:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 74)) {
          ptr = ctx->ParseMessage(_internal_mutable_stop_label_offset(), ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // .serialize_transport_catalogue.Color underlayer_color = 10;
      case 10:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 82)) {
          ptr = ctx->ParseMessage(_internal_mutable_underlayer_color(), ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // double underlayer_width = 11;
      case 11:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 89)) {
          _impl_.underlayer_width_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<double>(ptr);
          ptr += sizeof(double);
        } else
          goto handle_unusual;
        continue;
      // repeated .serialize_transport_catalogue.Color color_palette = 12;
      case 12:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 98)) {
          ptr -= 1;
          do {
            ptr += 1;
            ptr = ctx->ParseMessage(_internal_add_color_palette(), ptr);
            CHK_(ptr);
            if (!ctx->DataAvailable(ptr)) break;
          } while (::PROTOBUF_NAMESPACE_ID::internal::ExpectTag<98>(ptr));
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* RenderSettings::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:serialize_transport_catalogue.RenderSettings)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // double width = 1;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_width = this->_internal_width();
  uint64_t raw_width;
  memcpy(&raw_width, &tmp_width, sizeof(tmp_width));
  if (raw_width != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteDoubleToArray(1, this->_internal_width(), target);
  }

  // double height = 2;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_height = this->_internal_height();
  uint64_t raw_height;
  memcpy(&raw_height, &tmp_height, sizeof(tmp_height));
  if (raw_height != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteDoubleToArray(2, this->_internal_height(), target);
  }

  // double padding = 3;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_padding = this->_internal_padding();
  uint64_t raw_padding;
  memcpy(&raw_padding, &tmp_padding, sizeof(tmp_padding));
  if (raw_padding != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteDoubleToArray(3, this->_internal_padding(), target);
  }

  // double line_width = 4;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_line_width = this->_internal_line_width();
  uint64_t raw_line_width;
  memcpy(&raw_line_width, &tmp_line_width, sizeof(tmp_line_width));
  if (raw_line_width != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteDoubleToArray(4, this->_internal_line_width(), target);
  }

  // double stop_radius = 5;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_stop_radius = this->_internal_stop_radius();
  uint64_t raw_stop_radius;
  memcpy(&raw_stop_radius, &tmp_stop_radius, sizeof(tmp_stop_radius));
  if (raw_stop_radius != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteDoubleToArray(5, this->_internal_stop_radius(), target);
  }

  // double bus_label_font_size = 6;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_bus_label_font_size = this->_internal_bus_label_font_size();
  uint64_t raw_bus_label_font_size;
  memcpy(&raw_bus_label_font_size, &tmp_bus_label_font_size, sizeof(tmp_bus_label_font_size));
  if (raw_bus_label_font_size != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteDoubleToArray(6, this->_internal_bus_label_font_size(), target);
  }

  // .serialize_transport_catalogue.Point bus_label_offset = 7;
  if (this->_internal_has_bus_label_offset()) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(7, _Internal::bus_label_offset(this),
        _Internal::bus_label_offset(this).GetCachedSize(), target, stream);
  }

  // double stop_label_font_size = 8;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_stop_label_font_size = this->_internal_stop_label_font_size();
  uint64_t raw_stop_label_font_size;
  memcpy(&raw_stop_label_font_size, &tmp_stop_label_font_size, sizeof(tmp_stop_label_font_size));
  if (raw_stop_label_font_size != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteDoubleToArray(8, this->_internal_stop_label_font_size(), target);
  }

  // .serialize_transport_catalogue.Point stop_label_offset = 9;
  if (this->_internal_has_stop_label_offset()) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(9, _Internal::stop_label_offset(this),
        _Internal::stop_label_offset(this).GetCachedSize(), target, stream);
  }

  // .serialize_transport_catalogue.Color underlayer_color = 10;
  if (this->_internal_has_underlayer_color()) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(10, _Internal::underlayer_color(this),
        _Internal::underlayer_color(this).GetCachedSize(), target, stream);
  }

  // double underlayer_width = 11;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_underlayer_width = this->_internal_underlayer_width();
  uint64_t raw_underlayer_width;
  memcpy(&raw_underlayer_width, &tmp_underlayer_width, sizeof(tmp_underlayer_width));
  if (raw_underlayer_width != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteDoubleToArray(11, this->_internal_underlayer_width(), target);
  }

  // repeated .serialize_transport_catalogue.Color color_palette = 12;
  for (unsigned i = 0,
      n = static_cast<unsigned>(this->_internal_color_palette_size()); i < n; i++) {
    const auto& repfield = this->_internal_color_palette(i);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
        InternalWriteMessage(12, repfield, repfield.GetCachedSize(), target, stream);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:serialize_transport_catalogue.RenderSettings)
  return target;
}

size_t RenderSettings::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:serialize_transport_catalogue.RenderSettings)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // repeated .serialize_transport_catalogue.Color color_palette = 12;
  total_size += 1UL * this->_internal_color_palette_size();
  for (const auto& msg : this->_impl_.color_palette_) {
    total_size +=
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(msg);
  }

  // .serialize_transport_catalogue.Point bus_label_offset = 7;
  if (this->_internal_has_bus_label_offset()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(
        *_impl_.bus_label_offset_);
  }

  // .serialize_transport_catalogue.Point stop_label_offset = 9;
  if (this->_internal_has_stop_label_offset()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(
        *_impl_.stop_label_offset_);
  }

  // .serialize_transport_catalogue.Color underlayer_color = 10;
  if (this->_internal_has_underlayer_color()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(
        *_impl_.underlayer_color_);
  }

  // double width = 1;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_width = this->_internal_width();
  uint64_t raw_width;
  memcpy(&raw_width, &tmp_width, sizeof(tmp_width));
  if (raw_width != 0) {
    total_size += 1 + 8;
  }

  // double height = 2;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_height = this->_internal_height();
  uint64_t raw_height;
  memcpy(&raw_height, &tmp_height, sizeof(tmp_height));
  if (raw_height != 0) {
    total_size += 1 + 8;
  }

  // double padding = 3;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_padding = this->_internal_padding();
  uint64_t raw_padding;
  memcpy(&raw_padding, &tmp_padding, sizeof(tmp_padding));
  if (raw_padding != 0) {
    total_size += 1 + 8;
  }

  // double line_width = 4;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_line_width = this->_internal_line_width();
  uint64_t raw_line_width;
  memcpy(&raw_line_width, &tmp_line_width, sizeof(tmp_line_width));
  if (raw_line_width != 0) {
    total_size += 1 + 8;
  }

  // double stop_radius = 5;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_stop_radius = this->_internal_stop_radius();
  uint64_t raw_stop_radius;
  memcpy(&raw_stop_radius, &tmp_stop_radius, sizeof(tmp_stop_radius));
  if (raw_stop_radius != 0) {
    total_size += 1 + 8;
  }

  // double bus_label_font_size = 6;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_bus_label_font_size = this->_internal_bus_label_font_size();
  uint64_t raw_bus_label_font_size;
  memcpy(&raw_bus_label_font_size, &tmp_bus_label_font_size, sizeof(tmp_bus_label_font_size));
  if (raw_bus_label_font_size != 0) {
    total_size += 1 + 8;
  }

  // double stop_label_font_size = 8;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_stop_label_font_size = this->_internal_stop_label_font_size();
  uint64_t raw_stop_label_font_size;
  memcpy(&raw_stop_label_font_size, &tmp_stop_label_font_size, sizeof(tmp_stop_label_font_size));
  if (raw_stop_label_font_size != 0) {
    total_size += 1 + 8;
  }

  // double underlayer_width = 11;
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_underlayer_width = this->_internal_underlayer_width();
  uint64_t raw_underlayer_width;
  memcpy(&raw_underlayer_width, &tmp_underlayer_width, sizeof(tmp_underlayer_width));
  if (raw_underlayer_width != 0) {
    total_size += 1 + 8;
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData RenderSettings::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    RenderSettings::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*RenderSettings::GetClassData() const { return &_class_data_; }


void RenderSettings::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<RenderSettings*>(&to_msg);
  auto& from = static_cast<const RenderSettings&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:serialize_transport_catalogue.RenderSettings)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  _this->_impl_.color_palette_.MergeFrom(from._impl_.color_palette_);
  if (from._internal_has_bus_label_offset()) {
    _this->_internal_mutable_bus_label_offset()->::serialize_transport_catalogue::Point::MergeFrom(
        from._internal_bus_label_offset());
  }
  if (from._internal_has_stop_label_offset()) {
    _this->_internal_mutable_stop_label_offset()->::serialize_transport_catalogue::Point::MergeFrom(
        from._internal_stop_label_offset());
  }
  if (from._internal_has_underlayer_color()) {
    _this->_internal_mutable_underlayer_color()->::serialize_transport_catalogue::Color::MergeFrom(
        from._internal_underlayer_color());
  }
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_width = from._internal_width();
  uint64_t raw_width;
  memcpy(&raw_width, &tmp_width, sizeof(tmp_width));
  if (raw_width != 0) {
    _this->_internal_set_width(from._internal_width());
  }
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_height = from._internal_height();
  uint64_t raw_height;
  memcpy(&raw_height, &tmp_height, sizeof(tmp_height));
  if (raw_height != 0) {
    _this->_internal_set_height(from._internal_height());
  }
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_padding = from._internal_padding();
  uint64_t raw_padding;
  memcpy(&raw_padding, &tmp_padding, sizeof(tmp_padding));
  if (raw_padding != 0) {
    _this->_internal_set_padding(from._internal_padding());
  }
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_line_width = from._internal_line_width();
  uint64_t raw_line_width;
  memcpy(&raw_line_width, &tmp_line_width, sizeof(tmp_line_width));
  if (raw_line_width != 0) {
    _this->_internal_set_line_width(from._internal_line_width());
  }
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_stop_radius = from._internal_stop_radius();
  uint64_t raw_stop_radius;
  memcpy(&raw_stop_radius, &tmp_stop_radius, sizeof(tmp_stop_radius));
  if (raw_stop_radius != 0) {
    _this->_internal_set_stop_radius(from._internal_stop_radius());
  }
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_bus_label_font_size = from._internal_bus_label_font_size();
  uint64_t raw_bus_label_font_size;
  memcpy(&raw_bus_label_font_size, &tmp_bus_label_font_size, sizeof(tmp_bus_label_font_size));
  if (raw_bus_label_font_size != 0) {
    _this->_internal_set_bus_label_font_size(from._internal_bus_label_font_size());
  }
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_stop_label_font_size = from._internal_stop_label_font_size();
  uint64_t raw_stop_label_font_size;
  memcpy(&raw_stop_label_font_size, &tmp_stop_label_font_size, sizeof(tmp_stop_label_font_size));
  if (raw_stop_label_font_size != 0) {
    _this->_internal_set_stop_label_font_size(from._internal_stop_label_font_size());
  }
  static_assert(sizeof(uint64_t) == sizeof(double), "Code assumes uint64_t and double are the same size.");
  double tmp_underlayer_width = from._internal_underlayer_width();
  uint64_t raw_underlayer_width;
  memcpy(&raw_underlayer_width, &tmp_underlayer_width, sizeof(tmp_underlayer_width));
  if (raw_underlayer_width != 0) {
    _this->_internal_set_underlayer_width(from._internal_underlayer_width());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void RenderSettings::CopyFrom(const RenderSettings& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:serialize_transport_catalogue.RenderSettings)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool RenderSettings::IsInitialized() const {
  return true;
}

void RenderSettings::InternalSwap(RenderSettings* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  _impl_.color_palette_.InternalSwap(&other->_impl_.color_palette_);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(RenderSettings, _impl_.underlayer_width_)
      + sizeof(RenderSettings::_impl_.underlayer_width_)
      - PROTOBUF_FIELD_OFFSET(RenderSettings, _impl_.bus_label_offset_)>(
          reinterpret_cast<char*>(&_impl_.bus_label_offset_),
          reinterpret_cast<char*>(&other->_impl_.bus_label_offset_));
}

::PROTOBUF_NAMESPACE_ID::Metadata RenderSettings::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_map_5frenderer_2eproto_getter, &descriptor_table_map_5frenderer_2eproto_once,
      file_level_metadata_map_5frenderer_2eproto[0]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace serialize_transport_catalogue
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::serialize_transport_catalogue::RenderSettings*
Arena::CreateMaybeMessage< ::serialize_transport_catalogue::RenderSettings >(Arena* arena) {
  return Arena::CreateMessageInternal< ::serialize_transport_catalogue::RenderSettings >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
