#ifndef _XMLRPCVALUE_H_ // -*- C++ -*-
#define _XMLRPCVALUE_H_

#if defined(_MSC_VER)
#pragma warning(disable : 4786) // identifier was truncated in debug info
#endif

#ifndef MAKEDEPEND
#include <map>
#include <string>
#include <time.h>
#include <vector>
#endif

namespace XmlRpc {

//! RPC method arguments and results are represented by Values
//   should probably refcount them...
class XmlRpcValue {
public:
    enum Type {
        TypeInvalid,
        TypeBoolean,
        TypeInt,
        TypeDouble,
        TypeString,
        TypeDateTime,
        TypeBase64,
        TypeArray,
        TypeStruct
    };

    // Non-primitive types
    using BinaryData = std::vector<char>;
    using ValueArray = std::vector<XmlRpcValue>;
    using ValueStruct = std::map<std::string, XmlRpcValue>;

    //! Constructors
    XmlRpcValue() : _type(TypeInvalid) { _value.asBinary = 0; }
    XmlRpcValue(bool value) : _type(TypeBoolean) { _value.asBool = value; }
    XmlRpcValue(int value) : _type(TypeInt) { _value.asInt = value; }
    XmlRpcValue(double value) : _type(TypeDouble) { _value.asDouble = value; }

    XmlRpcValue(std::string const& value) : _type(TypeString) { _value.asString = new std::string(value); }

    XmlRpcValue(const char* value) : _type(TypeString) { _value.asString = new std::string(value); }

    XmlRpcValue(struct tm* value) : _type(TypeDateTime) { _value.asTime = new struct tm(*value); }

    XmlRpcValue(void* value, int nBytes) : _type(TypeBase64)
    {
        _value.asBinary = new BinaryData((char*)value, ((char*)value) + nBytes);
    }

    XmlRpcValue(ValueArray* valueArray) : _type(TypeArray) { _value.asArray = valueArray; }

    XmlRpcValue(ValueStruct* valueStruct) : _type(TypeStruct) { _value.asStruct = valueStruct; }

    //! Construct from xml, beginning at *offset chars into the string, updates offset
    XmlRpcValue(std::string const& xml, int* offset) : _type(TypeInvalid)
    {
        if (!fromXml(xml, offset)) _type = TypeInvalid;
    }

    //! Copy
    XmlRpcValue(XmlRpcValue const& rhs) : _type(TypeInvalid) { *this = rhs; }

    //! Destructor (make virtual if you want to subclass)
    /*virtual*/ ~XmlRpcValue() { invalidate(); }

    //! Erase the current value
    void clear() { invalidate(); }

    // Operators
    XmlRpcValue& operator=(XmlRpcValue const& rhs);
    XmlRpcValue& operator=(bool const& rhs) { return operator=(XmlRpcValue(rhs)); }
    XmlRpcValue& operator=(int const& rhs) { return operator=(XmlRpcValue(rhs)); }
    XmlRpcValue& operator=(double const& rhs) { return operator=(XmlRpcValue(rhs)); }
    XmlRpcValue& operator=(const char* rhs) { return operator=(XmlRpcValue(std::string(rhs))); }
    XmlRpcValue& operator=(ValueArray* valueArray);
    XmlRpcValue& operator=(ValueStruct* valueStruct);

    bool operator==(XmlRpcValue const& other) const;
    bool operator!=(XmlRpcValue const& other) const;

    operator bool&()
    {
        assertTypeOrInvalid(TypeBoolean);
        return _value.asBool;
    }
    operator int&()
    {
        assertTypeOrInvalid(TypeInt);
        return _value.asInt;
    }
    operator double&()
    {
        assertTypeOrInvalid(TypeDouble);
        return _value.asDouble;
    }
    operator std::string&()
    {
        assertTypeOrInvalid(TypeString);
        return *_value.asString;
    }
    operator BinaryData&()
    {
        assertTypeOrInvalid(TypeBase64);
        return *_value.asBinary;
    }
    operator struct tm&()
    {
        assertTypeOrInvalid(TypeDateTime);
        return *_value.asTime;
    }
    operator ValueArray&()
    {
        assertTypeOrInvalid(TypeArray);
        return *_value.asArray;
    }
    operator ValueStruct&()
    {
        assertTypeOrInvalid(TypeStruct);
        return *_value.asStruct;
    }

    operator const bool&() const
    {
        assertType(TypeBoolean);
        return _value.asBool;
    }
    operator const int&() const
    {
        assertType(TypeInt);
        return _value.asInt;
    }
    operator const double&() const
    {
        assertType(TypeDouble);
        return _value.asDouble;
    }
    operator const std::string&() const
    {
        assertType(TypeString);
        return *_value.asString;
    }
    operator const BinaryData&() const
    {
        assertType(TypeBase64);
        return *_value.asBinary;
    }
    operator const struct tm&() const
    {
        assertType(TypeDateTime);
        return *_value.asTime;
    }
    operator const ValueArray&() const
    {
        assertType(TypeArray);
        return *_value.asArray;
    }
    operator const ValueStruct&() const
    {
        assertType(TypeStruct);
        return *_value.asStruct;
    }

    void push_back(const XmlRpcValue& value)
    {
        assertArray(size() + 1);
        _value.asArray->at(size() - 1) = value;
    }

    XmlRpcValue const& operator[](int i) const
    {
        assertArray(i + 1);
        return _value.asArray->at(i);
    }

    XmlRpcValue& operator[](int i)
    {
        assertArray(i + 1);
        return _value.asArray->at(i);
    }

    XmlRpcValue& operator[](std::string const& k)
    {
        assertStruct();
        return (*_value.asStruct)[k];
    }

    XmlRpcValue const& operator[](std::string const& k) const
    {
        assertStruct();
        return (*_value.asStruct)[k];
    }

    XmlRpcValue& operator[](const char* k)
    {
        assertStruct();
        std::string s(k);
        return (*_value.asStruct)[s];
    }

    XmlRpcValue const& operator[](const char* k) const
    {
        assertStruct();
        std::string s(k);
        return (*_value.asStruct)[s];
    }

    // Accessors
    //! Return true if the value has been set to something.
    bool valid() const { return _type != TypeInvalid; }

    //! Return the type of the value stored. \see Type.
    Type const& getType() const { return _type; }

    //! Return the size for string, base64, array, and struct values.
    int size() const;

    //! Specify the size for array values. Array values will grow beyond this size if needed.
    void setSize(int size) { assertArray(size); }

    //! Check for the existence of a struct member by name.
    bool hasMember(const std::string& name) const;

    //! Decode xml. Destroys any existing value.
    bool fromXml(std::string const& valueXml, int* offset);

    //! Encode the Value in xml
    std::string toXml() const;

    //! Write the value (no xml encoding)
    std::ostream& write(std::ostream& os) const;

    // Formatting
    //! Return the format used to write double values.
    static std::string const& getDoubleFormat() { return _doubleFormat; }

    //! Specify the format used to write double values.
    static void setDoubleFormat(const char* f) { _doubleFormat = f; }

protected:
    // Clean up
    void invalidate();

    // Type checking
    void assertTypeOrInvalid(Type t);
    void assertType(Type t) const;
    void assertArray(int size) const;
    void assertArray(int size);
    void assertStruct() const;
    void assertStruct();

    // XML decoding
    bool boolFromXml(std::string const& valueXml, int* offset);
    bool intFromXml(std::string const& valueXml, int* offset);
    bool doubleFromXml(std::string const& valueXml, int* offset);
    bool stringFromXml(std::string const& valueXml, int* offset);
    bool timeFromXml(std::string const& valueXml, int* offset);
    bool binaryFromXml(std::string const& valueXml, int* offset);
    bool arrayFromXml(std::string const& valueXml, int* offset);
    bool structFromXml(std::string const& valueXml, int* offset);

    // XML encoding
    std::string boolToXml() const;
    std::string intToXml() const;
    std::string doubleToXml() const;
    std::string stringToXml() const;
    std::string timeToXml() const;
    std::string binaryToXml() const;
    std::string arrayToXml() const;
    std::string structToXml() const;

    // Format strings
    static std::string _doubleFormat;

    // Type tag and values
    Type _type;

    // At some point I will split off Arrays and Structs into
    // separate ref-counted objects for more efficient copying.
    union {
        bool asBool;
        int asInt;
        double asDouble;
        struct tm* asTime;
        std::string* asString;
        BinaryData* asBinary;
        ValueArray* asArray;
        ValueStruct* asStruct;
    } _value;
};

inline std::ostream&
operator<<(std::ostream& os, const XmlRpc::XmlRpcValue& v)
{
    return v.write(os);
}

} // namespace XmlRpc

/** \file
 */

#endif // _XMLRPCVALUE_H_
