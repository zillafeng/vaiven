#include "std.h"

#include <iostream>

#include "heap.h"
#include "runtime_error.h"

using namespace std;
using namespace vaiven;

void vaiven::init_std(Functions& funcs) {
  funcs.addNative("print", 1, (void*) print, false);
  funcs.addNative("list", 0, (void*) newList, true);
  funcs.addNative("append", 2, (void*) append, false);
  funcs.addNative("len", 1, (void*) len, false); // not pure because can throw
  funcs.addNative("assert", 1, (void*) assert, false);
  funcs.addNative("object", 0, (void*) object, true);
  funcs.addNative("keys", 1, (void*) keys, false); // not pure because can throw
  funcs.addNative("set", 3, (void*) set, false);
  funcs.addNative("get", 2, (void*) get, false); // not pure because can throw
  funcs.addNative("cmp", 2, (void*) cmp, true);
  funcs.addNative("toString", 1, (void*) toString, true);
}

Value vaiven::print(Value value) {
  if (value.isInt()) {
    cout << "Int: " << value.getInt() << endl << endl;
  } else {
    cout << toStringCpp(value) << endl << endl;
  }

  return Value();
}

Value vaiven::newList() {
  GcableList* list = globalHeap->newList();
  return Value(list);
}

Value vaiven::add(Value lhs, Value rhs) {
  if (lhs.isInt() && rhs.isInt()) {
    return Value(lhs.getInt() + rhs.getInt());
  } else if (lhs.isPtr() && lhs.getPtr()->getType() == GCABLE_TYPE_STRING) {
    if (rhs.isPtr() && rhs.getPtr()->getType() == GCABLE_TYPE_STRING) {
      GcableString* strleft = (GcableString*) lhs.getPtr();
      GcableString* strright = (GcableString*) rhs.getPtr();
      return addStrUnchecked(strleft, strright);
    }
  }
  expectedStrOrInt();
}

Value vaiven::addStrUnchecked(GcableString* lhs, GcableString* rhs) {
  GcableString* result = globalHeap->newString();
  result->str = lhs->str + rhs->str;
  return Value(result);
}

Value vaiven::append(Value lhs, Value rhs) {
  if (!lhs.isPtr()) {
    expectedListOrStr();
  }

  if (lhs.getPtr()->getType() == GCABLE_TYPE_LIST) {
    GcableList* list = (GcableList*) lhs.getPtr();
    list->list.push_back(rhs);
  } else if (lhs.isPtr() && lhs.getPtr()->getType() == GCABLE_TYPE_STRING) {
    if (rhs.isPtr() && rhs.getPtr()->getType() == GCABLE_TYPE_STRING) {
      GcableString* strleft = (GcableString*) lhs.getPtr();
      GcableString* strright = (GcableString*) rhs.getPtr();
      GcableString* result = globalHeap->newString();
      result->str = strleft->str + strright->str;
      return Value(result);
    } else {
      expectedStr();
    }
  } else {
    expectedListOrStr();
  }

  return Value();
}

Value vaiven::len(Value subject) {
  if (!subject.isPtr()) {
    expectedListOrStr();
  }

  if (subject.getPtr()->getType() == GCABLE_TYPE_LIST) {
    GcableList* list = (GcableList*) subject.getPtr();
    return Value((int) list->list.size());
  } else if (subject.getPtr()->getType() == GCABLE_TYPE_STRING) {
    GcableString* str = (GcableString*) subject.getPtr();
    return Value((int) str->str.size());
  } else {
    expectedListOrStr();
  }
}

Value vaiven::assert(Value expectation) {
  if (!expectation.isBool()) {
    expectedBool();
  }

  if (!expectation.getBool()) {
    // TODO custom message
    errString("assertion failed");
  }

  return Value();
}

Value vaiven::object() {
  return Value(globalHeap->newObject());
}

Value vaiven::keys(Value subject) {
  if (!subject.isPtr()) {
    expectedObj();
  }

  if (subject.getPtr()->getType() != GCABLE_TYPE_OBJECT) {
    expectedInt();
  }

  GcableObject* object = (GcableObject*) subject.getPtr();
  GcableList* list = globalHeap->newList();

  for (unordered_map<string, Value>::iterator it = object->properties.begin();
      it != object->properties.end();
      ++it) {
    GcableString* str = globalHeap->newString();
    str->str = it->first;
    list->list.push_back(Value(str));
  }

  return Value(list);
}

Value vaiven::set(Value subject, Value propOrIndex, Value value) {
  if (!subject.isPtr()) {
    expectedInt();
  }

  if (subject.getPtr()->getType() == GCABLE_TYPE_LIST) {
    GcableList* list = (GcableList*) subject.getPtr();
    if (propOrIndex.isInt()) {
      int index = propOrIndex.getInt();
      while (list->list.size() <= index) {
        list->list.push_back(Value());
      }
      list->list[index] = value;
    } else {
      expectedInt();
    }
  } else if (subject.getPtr()->getType() == GCABLE_TYPE_OBJECT) {
    GcableObject* object = (GcableObject*) subject.getPtr();
    if (propOrIndex.isPtr()
        && propOrIndex.getPtr()->getType() == GCABLE_TYPE_STRING) {
      GcableString* attrname = (GcableString*) propOrIndex.getPtr();
      object->properties[attrname->str] = value;
    } else {
      expectedInt();
    }
  } else {
    expectedInt();
  }

  return Value();
}

Value vaiven::get(Value subject, Value propOrIndex) {
  if (!subject.isPtr()) {
    expectedInt();
  }

  if (subject.getPtr()->getType() == GCABLE_TYPE_LIST) {
    GcableList* list = (GcableList*) subject.getPtr();
    if (propOrIndex.isInt()) {
      int index = propOrIndex.getInt();
      if (list->list.size() <= index) {
        return Value();
      }
      return list->list[index];
    } else {
      expectedInt();
    }
  } else if (subject.getPtr()->getType() == GCABLE_TYPE_OBJECT) {
    GcableObject* object = (GcableObject*) subject.getPtr();
    if (propOrIndex.isPtr()
        && propOrIndex.getPtr()->getType() == GCABLE_TYPE_STRING) {
      GcableString* attrname = (GcableString*) propOrIndex.getPtr();
      if (object->properties.find(attrname->str) == object->properties.end()) {
        return Value();
      }
      return object->properties[attrname->str];
    } else {
      expectedInt();
    }
  } else {
    expectedInt();
  }

  return Value();
}

Value vaiven::cmp(Value a, Value b) {
  return Value(cmpUnboxed(a, b));
}

Value vaiven::inverseCmp(Value a, Value b) {
  return Value(inverseCmpUnboxed(a, b));
}

bool vaiven::cmpUnboxed(Value a, Value b) {
  if (a.isPtr() && b.isPtr()
      && a.getPtr()->getType() == GCABLE_TYPE_STRING
      && b.getPtr()->getType() == GCABLE_TYPE_STRING) {
    return cmpStrUnchecked((GcableString*) a.getPtr(), (GcableString*) b.getPtr());
  }
  return a.getRaw() == b.getRaw();
}

bool vaiven::cmpStrUnchecked(GcableString* a, GcableString* b) {
  return a->str == b->str;
}

bool vaiven::inverseCmpUnboxed(Value a, Value b) {
  return !cmpUnboxed(a, b);
}

bool vaiven::inverseCmpStrUnchecked(GcableString* a, GcableString* b) {
  return !cmpStrUnchecked(a, b);
}

Value vaiven::toString(Value subject) {
  if (subject.isPtr() && subject.getPtr()->getType() == GCABLE_TYPE_STRING) {
    return subject;
  }

  GcableString* str = globalHeap->newString();
  str->str = toStringCpp(subject);
  return Value(str);
}

string vaiven::toStringCpp(Value subject) {
  if (subject.isPtr() && subject.getPtr()->getType() == GCABLE_TYPE_STRING) {
    return ((GcableString*) subject.getPtr())->str;
  }
  
  if (subject.isPtr() && subject.getPtr()->getType() == GCABLE_TYPE_LIST) {
    GcableList* list = (GcableList*) subject.getPtr();
    string result = "[";
    for (vector<Value>::iterator it = list->list.begin(); it != list->list.end(); ++it) {
      if (it != list->list.begin()) {
        result += ", ";
      }
      result += toStringCpp(*it);
    }
    result += "]";
    return result;
  }
  
  if (subject.isPtr() && subject.getPtr()->getType() == GCABLE_TYPE_OBJECT) {
    GcableObject* object = (GcableObject*) subject.getPtr();
    string result = "{";
    for (unordered_map<string, Value>::iterator it = object->properties.begin(); it != object->properties.end(); ++it) {
      if (it != object->properties.begin()) {
        result += ", ";
      }
      Value val = it->second;
      result += it->first + ":" + toStringCpp(val);
    }
    result += "}";
    return result;
  }
  
  if (subject.isBool() && subject.getBool()) {
    return "true";
  }
  
  if (subject.isBool() && !subject.getBool()) {
    return "false";
  }
  
  if (subject.isVoid()) {
    return "void";
  }

  return std::to_string(subject.getInt());
}
