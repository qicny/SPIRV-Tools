// Copyright (c) 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef LIBSPIRV_OPT_CONSTANTS_H_
#define LIBSPIRV_OPT_CONSTANTS_H_

#include <cinttypes>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "make_unique.h"
#include "module.h"
#include "type_manager.h"
#include "types.h"

namespace spvtools {
namespace opt {
namespace analysis {

// Class hierarchy to represent the normal constants defined through
// OpConstantTrue, OpConstantFalse, OpConstant, OpConstantNull and
// OpConstantComposite instructions.
// TODO(qining): Add class for constants defined with OpConstantSampler.
class Constant;
class ScalarConstant;
class IntConstant;
class FloatConstant;
class BoolConstant;
class CompositeConstant;
class StructConstant;
class VectorConstant;
class ArrayConstant;
class NullConstant;

// Abstract class for a SPIR-V constant. It has a bunch of As<subclass> methods,
// which is used as a way to probe the actual <subclass>
class Constant {
 public:
  Constant() = delete;
  virtual ~Constant() {}

  // Make a deep copy of this constant.
  virtual std::unique_ptr<Constant> Copy() const = 0;

  // reflections
  virtual ScalarConstant* AsScalarConstant() { return nullptr; }
  virtual IntConstant* AsIntConstant() { return nullptr; }
  virtual FloatConstant* AsFloatConstant() { return nullptr; }
  virtual BoolConstant* AsBoolConstant() { return nullptr; }
  virtual CompositeConstant* AsCompositeConstant() { return nullptr; }
  virtual StructConstant* AsStructConstant() { return nullptr; }
  virtual VectorConstant* AsVectorConstant() { return nullptr; }
  virtual ArrayConstant* AsArrayConstant() { return nullptr; }
  virtual NullConstant* AsNullConstant() { return nullptr; }

  virtual const ScalarConstant* AsScalarConstant() const { return nullptr; }
  virtual const IntConstant* AsIntConstant() const { return nullptr; }
  virtual const FloatConstant* AsFloatConstant() const { return nullptr; }
  virtual const BoolConstant* AsBoolConstant() const { return nullptr; }
  virtual const CompositeConstant* AsCompositeConstant() const {
    return nullptr;
  }
  virtual const StructConstant* AsStructConstant() const { return nullptr; }
  virtual const VectorConstant* AsVectorConstant() const { return nullptr; }
  virtual const ArrayConstant* AsArrayConstant() const { return nullptr; }
  virtual const NullConstant* AsNullConstant() const { return nullptr; }

  const Type* type() const { return type_; }

 protected:
  Constant(const Type* ty) : type_(ty) {}

  // The type of this constant.
  const Type* type_;
};

// Abstract class for scalar type constants.
class ScalarConstant : public Constant {
 public:
  ScalarConstant() = delete;
  ScalarConstant* AsScalarConstant() override { return this; }
  const ScalarConstant* AsScalarConstant() const override { return this; }

  // Returns a const reference of the value of this constant in 32-bit words.
  virtual const std::vector<uint32_t>& words() const { return words_; }

 protected:
  ScalarConstant(const Type* ty, const std::vector<uint32_t>& w)
      : Constant(ty), words_(w) {}
  ScalarConstant(const Type* ty, std::vector<uint32_t>&& w)
      : Constant(ty), words_(std::move(w)) {}
  std::vector<uint32_t> words_;
};

// Integer type constant.
class IntConstant : public ScalarConstant {
 public:
  IntConstant(const Integer* ty, const std::vector<uint32_t>& w)
      : ScalarConstant(ty, w) {}
  IntConstant(const Integer* ty, std::vector<uint32_t>&& w)
      : ScalarConstant(ty, std::move(w)) {}

  IntConstant* AsIntConstant() override { return this; }
  const IntConstant* AsIntConstant() const override { return this; }

  // Make a copy of this IntConstant instance.
  std::unique_ptr<IntConstant> CopyIntConstant() const {
    return MakeUnique<IntConstant>(type_->AsInteger(), words_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyIntConstant().release());
  }
};

// Float type constant.
class FloatConstant : public ScalarConstant {
 public:
  FloatConstant(const Float* ty, const std::vector<uint32_t>& w)
      : ScalarConstant(ty, w) {}
  FloatConstant(const Float* ty, std::vector<uint32_t>&& w)
      : ScalarConstant(ty, std::move(w)) {}

  FloatConstant* AsFloatConstant() override { return this; }
  const FloatConstant* AsFloatConstant() const override { return this; }

  // Make a copy of this FloatConstant instance.
  std::unique_ptr<FloatConstant> CopyFloatConstant() const {
    return MakeUnique<FloatConstant>(type_->AsFloat(), words_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyFloatConstant().release());
  }
};

// Bool type constant.
class BoolConstant : public ScalarConstant {
 public:
  BoolConstant(const Bool* ty, bool v)
      : ScalarConstant(ty, {static_cast<uint32_t>(v)}), value_(v) {}

  BoolConstant* AsBoolConstant() override { return this; }
  const BoolConstant* AsBoolConstant() const override { return this; }

  // Make a copy of this BoolConstant instance.
  std::unique_ptr<BoolConstant> CopyBoolConstant() const {
    return MakeUnique<BoolConstant>(type_->AsBool(), value_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyBoolConstant().release());
  }

  bool value() const { return value_; }

 private:
  bool value_;
};

// Abstract class for composite constants.
class CompositeConstant : public Constant {
 public:
  CompositeConstant() = delete;
  CompositeConstant* AsCompositeConstant() override { return this; }
  const CompositeConstant* AsCompositeConstant() const override { return this; }

  // Returns a const reference of the components holded in this composite
  // constant.
  virtual const std::vector<const Constant*>& GetComponents() const {
    return components_;
  }

 protected:
  CompositeConstant(const Type* ty) : Constant(ty), components_() {}
  CompositeConstant(const Type* ty,
                    const std::vector<const Constant*>& components)
      : Constant(ty), components_(components) {}
  CompositeConstant(const Type* ty, std::vector<const Constant*>&& components)
      : Constant(ty), components_(std::move(components)) {}
  std::vector<const Constant*> components_;
};

// Struct type constant.
class StructConstant : public CompositeConstant {
 public:
  StructConstant(const Struct* ty) : CompositeConstant(ty) {}
  StructConstant(const Struct* ty,
                 const std::vector<const Constant*>& components)
      : CompositeConstant(ty, components) {}
  StructConstant(const Struct* ty, std::vector<const Constant*>&& components)
      : CompositeConstant(ty, std::move(components)) {}

  StructConstant* AsStructConstant() override { return this; }
  const StructConstant* AsStructConstant() const override { return this; }

  // Make a copy of this StructConstant instance.
  std::unique_ptr<StructConstant> CopyStructConstant() const {
    return MakeUnique<StructConstant>(type_->AsStruct(), components_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyStructConstant().release());
  }
};

// Vector type constant.
class VectorConstant : public CompositeConstant {
 public:
  VectorConstant(const Vector* ty)
      : CompositeConstant(ty), component_type_(ty->element_type()) {}
  VectorConstant(const Vector* ty,
                 const std::vector<const Constant*>& components)
      : CompositeConstant(ty, components),
        component_type_(ty->element_type()) {}
  VectorConstant(const Vector* ty, std::vector<const Constant*>&& components)
      : CompositeConstant(ty, std::move(components)),
        component_type_(ty->element_type()) {}

  VectorConstant* AsVectorConstant() override { return this; }
  const VectorConstant* AsVectorConstant() const override { return this; }

  // Make a copy of this VectorConstant instance.
  std::unique_ptr<VectorConstant> CopyVectorConstant() const {
    auto another = MakeUnique<VectorConstant>(type_->AsVector());
    another->components_.insert(another->components_.end(), components_.begin(),
                                components_.end());
    return another;
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyVectorConstant().release());
  }

  const Type* component_type() { return component_type_; }

 private:
  const Type* component_type_;
};

// Array type constant.
class ArrayConstant : public CompositeConstant {
 public:
  ArrayConstant(const Array* ty) : CompositeConstant(ty) {}
  ArrayConstant(const Array* ty, const std::vector<const Constant*>& components)
      : CompositeConstant(ty, components) {}
  ArrayConstant(const Array* ty, std::vector<const Constant*>&& components)
      : CompositeConstant(ty, std::move(components)) {}

  ArrayConstant* AsArrayConstant() override { return this; }
  const ArrayConstant* AsArrayConstant() const override { return this; }

  // Make a copy of this ArrayConstant instance.
  std::unique_ptr<ArrayConstant> CopyArrayConstant() const {
    return MakeUnique<ArrayConstant>(type_->AsArray(), components_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyArrayConstant().release());
  }
};

// Null type constant.
class NullConstant : public Constant {
 public:
  NullConstant(const Type* ty) : Constant(ty) {}
  NullConstant* AsNullConstant() override { return this; }
  const NullConstant* AsNullConstant() const override { return this; }

  // Make a copy of this NullConstant instance.
  std::unique_ptr<NullConstant> CopyNullConstant() const {
    return MakeUnique<NullConstant>(type_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyNullConstant().release());
  }
};

class IRContext;

// Hash function for Constant instances. Use the structure of the constant as
// the key.
struct ConstantHash {
  void add_pointer(std::u32string* h, const void* p) const {
    uint64_t ptr_val = reinterpret_cast<uint64_t>(p);
    h->push_back(static_cast<uint32_t>(ptr_val >> 32));
    h->push_back(static_cast<uint32_t>(ptr_val));
  }

  size_t operator()(const Constant* const_val) const {
    std::u32string h;
    add_pointer(&h, const_val->type());
    if (const auto scalar = const_val->AsScalarConstant()) {
      for (const auto& w : scalar->words()) {
        h.push_back(w);
      }
    } else if (const auto composite = const_val->AsCompositeConstant()) {
      for (const auto& c : composite->GetComponents()) {
        add_pointer(&h, c);
      }
    } else if (const_val->AsNullConstant()) {
      h.push_back(0);
    } else {
      assert(
          false &&
          "Tried to compute the hash value of an invalid Constant instance.");
    }

    return std::hash<std::u32string>()(h);
  }
};

// Equality comparison structure for two constants.
struct ConstantEqual {
  bool operator()(const Constant* c1, const Constant* c2) const {
    if (c1->type() != c2->type()) {
      return false;
    }

    if (const auto& s1 = c1->AsScalarConstant()) {
      const auto& s2 = c2->AsScalarConstant();
      return s2 && s1->words() == s2->words();
    } else if (const auto& composite1 = c1->AsCompositeConstant()) {
      const auto& composite2 = c2->AsCompositeConstant();
      return composite2 &&
             composite1->GetComponents() == composite2->GetComponents();
    } else if (c1->AsNullConstant())
      return c2->AsNullConstant() != nullptr;
    else
      assert(false && "Tried to compare two invalid Constant instances.");
    return false;
  }
};

// This class represents a pool of constants.
class ConstantManager {
 public:
  ConstantManager(ir::IRContext* ctx) : ctx_(ctx) {}

  ir::IRContext* context() const { return ctx_; }

  // Gets or creates a unique Constant instance of type |type| and a vector of
  // constant defining words |words|. If a Constant instance existed already in
  // the constant pool, it returns a pointer to it.  Otherwise, it creates one
  // using CreateConstant. If a new Constant instance cannot be created, it
  // returns nullptr.
  const Constant* GetConstant(
      const Type* type, const std::vector<uint32_t>& literal_words_or_ids);

  // Gets or creates a Constant instance to hold the constant value of the given
  // instruction. It returns a pointer to a Constant instance or nullptr if it
  // could not create the constant.
  const Constant* GetConstantFromInst(ir::Instruction* inst);

  // Gets or creates a constant defining instruction for the given Constant |c|.
  // If |c| had already been defined, it returns a pointer to the existing
  // declaration. Otherwise, it calls BuildInstructionAndAddToModule. If the
  // optional |pos| is given, it will insert any newly created instructions at
  // the given instruction iterator position. Otherwise, it inserts the new
  // instruction at the end of the current module's types section.
  ir::Instruction* GetDefiningInstruction(
      const Constant* c, ir::Module::inst_iterator* pos = nullptr);

  // Creates a constant defining instruction for the given Constant instance
  // and inserts the instruction at the position specified by the given
  // instruction iterator. Returns a pointer to the created instruction if
  // succeeded, otherwise returns a null pointer. The instruction iterator
  // points to the same instruction before and after the insertion. This is the
  // only method that actually manages id creation/assignment and instruction
  // creation/insertion for a new Constant instance.
  //
  // |type_id| is an optional argument for disambiguating equivalent types. If
  // |type_id| is specified, it is used as the type of the constant. Otherwise
  // the type of the constant is derived by getting an id from the type manager
  // for |c|.
  ir::Instruction* BuildInstructionAndAddToModule(
      const Constant* c, ir::Module::inst_iterator* pos, uint32_t type_id = 0);

  // A helper function to get the result type of the given instruction. Returns
  // nullptr if the instruction does not have a type id (type id is 0).
  Type* GetType(const ir::Instruction* inst) const;

  // A helper function to get the collected normal constant with the given id.
  // Returns the pointer to the Constant instance in case it is found.
  // Otherwise, it returns a null pointer.
  const Constant* FindDeclaredConstant(uint32_t id) const {
    auto iter = id_to_const_val_.find(id);
    return (iter != id_to_const_val_.end()) ? iter->second : nullptr;
  }

  // A helper function to get the id of a collected constant with the pointer
  // to the Constant instance. Returns 0 in case the constant is not found.
  uint32_t FindDeclaredConstant(const Constant* c) const {
    auto iter = const_val_to_id_.find(c);
    return (iter != const_val_to_id_.end()) ? iter->second : 0;
  }

  // Returns the canonical constant that has the same structure and value as the
  // given Constant |cst|. If none is found, it returns nullptr.
  const Constant* FindConstant(const Constant* c) const {
    auto it = const_pool_.find(c);
    return (it != const_pool_.end()) ? *it : nullptr;
  }

  // Registers a new constant |cst| in the constant pool. If the constant
  // existed already, it returns a pointer to the previously existing Constant
  // in the pool. Otherwise, it returns |cst|.
  const Constant* RegisterConstant(const Constant* cst) {
    auto ret = const_pool_.insert(cst);
    return *ret.first;
  }

  // A helper function to get a vector of Constant instances with the specified
  // ids. If it can not find the Constant instance for any one of the ids,
  // it returns an empty vector.
  std::vector<const Constant*> GetConstantsFromIds(
      const std::vector<uint32_t>& ids) const;

  // Records a mapping between |inst| and the constant value generated by it.
  // It returns true if a new Constant was successfully mapped, false if |inst|
  // generates no constant values.
  bool MapInst(ir::Instruction* inst) {
    if (auto cst = GetConstantFromInst(inst)) {
      MapConstantToInst(cst, inst);
      return true;
    }
    return false;
  }

  // Records a new mapping between |inst| and |const_value|. This updates the
  // two mappings |id_to_const_val_| and |const_val_to_id_|.
  void MapConstantToInst(const Constant* const_value, ir::Instruction* inst) {
    const_val_to_id_[const_value] = inst->result_id();
    id_to_const_val_[inst->result_id()] = const_value;
  }

 private:
  // Creates a Constant instance with the given type and a vector of constant
  // defining words. Returns a unique pointer to the created Constant instance
  // if the Constant instance can be created successfully. To create scalar
  // type constants, the vector should contain the constant value in 32 bit
  // words and the given type must be of type Bool, Integer or Float. To create
  // composite type constants, the vector should contain the component ids, and
  // those component ids should have been recorded before as Normal Constants.
  // And the given type must be of type Struct, Vector or Array. When creating
  // VectorType Constant instance, the components must be scalars of the same
  // type, either Bool, Integer or Float. If any of the rules above failed, the
  // creation will fail and nullptr will be returned. If the vector is empty,
  // a NullConstant instance will be created with the given type.
  const Constant* CreateConstant(
      const Type* type,
      const std::vector<uint32_t>& literal_words_or_ids) const;

  // Creates an instruction with the given result id to declare a constant
  // represented by the given Constant instance. Returns an unique pointer to
  // the created instruction if the instruction can be created successfully.
  // Otherwise, returns a null pointer.
  //
  // |type_id| is an optional argument for disambiguating equivalent types. If
  // |type_id| is specified, it is used as the type of the constant. Otherwise
  // the type of the constant is derived by getting an id from the type manager
  // for |c|.
  std::unique_ptr<ir::Instruction> CreateInstruction(
      uint32_t result_id, const Constant* c, uint32_t type_id = 0) const;

  // Creates an OpConstantComposite instruction with the given result id and
  // the CompositeConst instance which represents a composite constant. Returns
  // an unique pointer to the created instruction if succeeded. Otherwise
  // returns a null pointer.
  //
  // |type_id| is an optional argument for disambiguating equivalent types. If
  // |type_id| is specified, it is used as the type of the constant. Otherwise
  // the type of the constant is derived by getting an id from the type manager
  // for |c|.
  std::unique_ptr<ir::Instruction> CreateCompositeInstruction(
      uint32_t result_id, const CompositeConstant* cc,
      uint32_t type_id = 0) const;

  // IR context that owns this constant manager.
  ir::IRContext* ctx_;

  // A mapping from the result ids of Normal Constants to their
  // Constant instances. All Normal Constants in the module, either
  // existing ones before optimization or the newly generated ones, should have
  // their Constant instance stored and their result id registered in this map.
  std::unordered_map<uint32_t, const Constant*> id_to_const_val_;

  // A mapping from the Constant instance of Normal Constants to their
  // result id in the module. This is a mirror map of |id_to_const_val_|. All
  // Normal Constants that defining instructions in the module should have
  // their Constant and their result id registered here.
  std::unordered_map<const Constant*, uint32_t> const_val_to_id_;

  // The constant pool.  All created constants are registered here.
  std::unordered_set<const Constant*, ConstantHash, ConstantEqual> const_pool_;
};

}  // namespace analysis
}  // namespace opt
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_CONSTANTS_H_
