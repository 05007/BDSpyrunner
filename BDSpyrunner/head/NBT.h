#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <variant>

#include "string_span.h"

class IDataInput;
class IDataOutput;
class PrintStream;

struct TagMemoryChunk {
  size_t m_cap, m_size;
  std::unique_ptr<unsigned char[]> m_data;

  template <typename T> __declspec(dllimport) T *alloc(unsigned long);
  __declspec(dllimport) TagMemoryChunk copy() const;
  __declspec(dllimport) bool operator!=(TagMemoryChunk const &) const;
  inline size_t size() const { return m_size; }
};

class __declspec(dllimport) Tag {
public:
  enum Type {
    End       = 0,
    Byte      = 1,
    Short     = 2,
    Int       = 3,
    Int64     = 4,
    Float     = 5,
    Double    = 6,
    ByteArray = 7,
    String    = 8,
    List      = 9,
    Compound  = 10,
  };
  virtual ~Tag();
  virtual void deleteChildren();
  virtual void write(IDataOutput &) const = 0;
  virtual void load(IDataInput &)         = 0;
  virtual std::string toString() const    = 0;
  virtual Tag::Type getId() const         = 0;
  virtual bool equals(Tag const &) const;
  virtual void print(PrintStream &) const;
  virtual void print(std::string const &, PrintStream &) const;
  virtual std::unique_ptr<Tag> copy() const;
  virtual std::uint64_t hash() const;
};

class __declspec(dllimport) EndTag : public Tag {
public:
  virtual ~EndTag() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 0
  virtual bool equals(Tag const &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;
};

class __declspec(dllimport) ByteTag : public Tag {
public:
  unsigned char value;
  virtual ~ByteTag() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 1
  virtual bool equals(Tag const &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;
};

class __declspec(dllimport) ShortTag : public Tag {
public:
  short value;
  virtual ~ShortTag() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 2
  virtual bool equals(Tag const &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;
};

class __declspec(dllimport) IntTag : public Tag {
public:
  int32_t value;
  virtual ~IntTag() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 3
  virtual bool equals(Tag const &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;
};

class __declspec(dllimport) Int64Tag : public Tag {
public:
  int64_t value;
  virtual ~Int64Tag() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 4
  virtual bool equals(Tag const &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;
};

class __declspec(dllimport) FloatTag : public Tag {
public:
  float value;
  virtual ~FloatTag() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 5
  virtual bool equals(Tag const &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;
};

class __declspec(dllimport) DoubleTag : public Tag {
public:
  double value;
  virtual ~DoubleTag() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 6
  virtual bool equals(Tag const &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;
};

class __declspec(dllimport) ByteArrayTag : public Tag {
public:
  TagMemoryChunk value;
  virtual ~ByteArrayTag() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 7
  virtual bool equals(Tag const &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;
};

class __declspec(dllimport) StringTag : public Tag {
public:
  std::string value;
  virtual ~StringTag() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 8
  virtual bool equals(Tag const &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;
};

class __declspec(dllimport) ListTag : public Tag {
public:
  std::vector<std::unique_ptr<Tag>> value;
  virtual ~ListTag() override;
  virtual void deleteChildren() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 9
  virtual bool equals(Tag const &) const override;
  virtual void print(std::string const &, PrintStream &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;

  class CompoundTag const *getCompound(unsigned __int64) const;

  std::string const &getString(int) const;
  void add(std::unique_ptr<Tag>);
  double getDouble(int) const;
  float getFloat(int) const;
  int getInt(int) const;
};

class __declspec(dllimport) CompoundTag : public Tag {
public:
  std::map<std::string, class CompoundTagVariant> value;
  //virtual ~CompoundTag() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 10
  virtual bool equals(Tag const &) const override;
  virtual void print(std::string const &, PrintStream &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;

  bool contains(gsl::cstring_span<>, enum Tag::Type) const;
  bool contains(gsl::cstring_span<>) const;

  bool remove(gsl::cstring_span<>);

  void append(CompoundTag const &);

  Tag *put(std::string, std::unique_ptr<Tag>);
  CompoundTag *putCompound(std::string, std::unique_ptr<CompoundTag>);
  CompoundTag &putCompound(std::string, CompoundTag);
  std::string &putString(std::string, std::string);
  void putBoolean(std::string, bool);
  unsigned char &putByte(std::string, unsigned char);
  int &putInt(std::string, int);
  int64_t &putInt64(std::string, int64_t);
  short &putShort(std::string, short);
  float &putFloat(std::string, float);

  std::string const &getString(gsl::cstring_span<>) const;
  struct TagMemoryChunk const &getByteArray(gsl::cstring_span<>) const;
  ListTag const *getList(gsl::cstring_span<>) const;
  ListTag *getList(gsl::cstring_span<>);
  CompoundTag const *getCompound(gsl::cstring_span<>) const;
  CompoundTag *getCompound(gsl::cstring_span<>);
  bool getBoolean(gsl::cstring_span<>) const;
  int64_t getInt64(gsl::cstring_span<>) const;
  short getShort(gsl::cstring_span<>) const;
  float getFloat(gsl::cstring_span<>) const;
  unsigned char getByte(gsl::cstring_span<>) const;
  int getInt(gsl::cstring_span<>) const;

  void deepCopy(CompoundTag const &);
  std::unique_ptr<CompoundTag> clone(void) const;
};

class __declspec(dllimport) IntArrayTag : public Tag {
public:
  TagMemoryChunk value;
  virtual ~IntArrayTag() override;
  virtual void write(IDataOutput &) const override;
  virtual void load(IDataInput &) override;
  virtual std::string toString() const override;
  virtual Tag::Type getId() const override; // 11
  virtual bool equals(Tag const &) const override;
  virtual std::unique_ptr<Tag> copy() const override;
  virtual std::uint64_t hash() const override;
};

class CompoundTagVariant : std::variant<
                               EndTag, ByteTag, ShortTag, IntTag, Int64Tag, FloatTag, DoubleTag, ByteArrayTag,
                               StringTag, ListTag, CompoundTag, IntArrayTag> {
  using std::variant<
      EndTag, ByteTag, ShortTag, IntTag, Int64Tag, FloatTag, DoubleTag, ByteArrayTag, StringTag, ListTag, CompoundTag,
      IntArrayTag>::variant;
};