#pragma once

#include <sstream>
#include <string>
#include <map>
#include <vector>

#include <QDebug>
#include <iostream>
#include <vector>

#include "Idptr.h"
#include "Constheader.h"

std::ostream& operator<<(std::stringstream& file, const std::vector<int> & content);
std::istream& operator>>(std::stringstream& file, std::vector<int> & content);

template<class T>
inline std::stringstream& WrapWriteMessage(std::stringstream& file, T& to_write)
{
    file << to_write;
    return file;
}

inline std::stringstream& WrapWriteMessage(std::stringstream& file, const std::string& to_write)
{
    file << static_cast<quint32>(to_write.size()) << " ";
    file << to_write;
    return file;
}

inline std::stringstream& WrapWriteMessage(std::stringstream& file, std::string& to_write)
{
    file << static_cast<quint32>(to_write.size()) << " ";
    file << to_write;
    return file;
}

template<class KeyType, class ValueType>
inline std::stringstream& WrapWriteMessage(
        std::stringstream& file,
        std::map<KeyType, ValueType>& to_write)
{
    file << to_write.size() << " ";
    for (auto it = to_write.begin(); it != to_write.end(); ++it)
    {
        WrapWriteMessage(file, it->first) << " ";
        WrapWriteMessage(file, it->second) << " ";
    }
    return file;
}

template<class T>
inline std::stringstream& WrapReadMessage(std::stringstream& file, T& to_read)
{
    file >> to_read;
    return file;
}

inline std::stringstream& WrapReadMessage(std::stringstream& file, std::string& to_read)
{
    quint32 size;
    file >> size;

    char c;
    file.get(c);
    if (c != ' ')
    {
        SYSTEM_STREAM << "Error: corrupted string info" << std::endl;
        return file;
    }

    if (size == 0)
    {
        to_read.clear();
        return file;
    }
    to_read.resize(size);
    for (quint32 i = 0; i < size; ++i)
    {
        char cr;
        file.get(cr);
        to_read[i] = cr;
    }

  //  SYSTEM_STREAM << "Final string: '" << to_read << "'" << std::endl;

    return file;
}

template<class KeyType, class ValueType>
inline std::stringstream& WrapReadMessage(
        std::stringstream& file,
        std::map<KeyType, ValueType>& to_read)
{
    quint32 size;
    file >> size;
    //qDebug() << size;
    if (size == 0)
    {
        return file;
    }
    for (quint32 i = 0; i < size; ++i)
    {
        KeyType key;
        WrapReadMessage(file, key);
        ValueType value;
        WrapReadMessage(file, value);
        to_read[key] = value;
    }
    return file;
}
