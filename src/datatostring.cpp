#include <UtilsQt/datatostring.h>

#include <QVector>
#include <QChar>
#include <string>
#include <optional>
#include <cassert>


namespace {
size_t hexStr(const void* data, size_t len, char* out, size_t outSz)
{
    size_t needSz = len ? len * 3 : 1;

    if (!outSz)
        return needSz;

    assert(data);
    assert(out);
    assert(outSz >= needSz);

    if (!len) {
        *out = 0;
        return 1;
    }

    static const char characters[] = "0123456789ABCDEF";

    const char* it = static_cast<const char*>(data);
    const char* itEnd = it + len;
    char* outIt = out;

    while (it != itEnd) {
        *outIt++ = characters[(*it >> 4) & 0x0F];
        *outIt++ = characters[*it & 0x0F];
        *outIt++ = ' ';
        it++;
    }

    *--outIt = 0;

    return needSz;
}

void addNonPrintData(QString& result, std::string& tempBuffer, int start, int end, const void* data) {
    size_t needSz = hexStr((const char*)data + start, (end - start), nullptr, 0) + 2;
    tempBuffer.resize(needSz - 1);
    hexStr((const char*)data + start, (end - start), tempBuffer.data() + 1, tempBuffer.size() + 1 - 2);
    tempBuffer[0] = '<';
    tempBuffer[tempBuffer.size()-1] = '>';
    result += QString::fromStdString(tempBuffer);
}
} // namespace


QString dataToString(const void* data, size_t sz)
{
    if (!sz)
        return {};

    std::string tempBuffer;

    QVector<QPair<int, int>> ranges;
    auto str = QString::fromLatin1((const char*)data, sz);

    int start = -1;
    int len = 0;
    int i = 0;
    bool isPrintablePart = false;

    for (auto it = str.cbegin(), itEnd = str.cend() + 1; it != itEnd; it++, i++) {
        if (isPrintablePart) {
            if (it->isPrint() && *it != QChar('\xFF')) {
                len++;
            } else {
                isPrintablePart = false;
                ranges.append(QPair<int, int>(start, len));
            }
        } else {
            if (it->isPrint() && *it != QChar('\xFF')) {
                start = i;
                len = 1;
                isPrintablePart = true;
            }
        }
    }

    QString result;
    if (start == -1) {
        addNonPrintData(result, tempBuffer, 0, sz, data);
        return result;
    } else if (start > 0) {
        addNonPrintData(result, tempBuffer, 0, ranges.first().first, data);
    }

    for (int i = 0; i < ranges.size(); i++) {
        QPair<int, int> currentRange = ranges.at(i);
        std::optional<QPair<int, int>> nextRange = (i == ranges.size() - 1) ? std::nullopt : std::optional<QPair<int, int>>(ranges.at(i+1));
        result += str.mid(currentRange.first, currentRange.second);

        if (nextRange) {
            int rawStart = currentRange.first + currentRange.second;
            int rawEnd = nextRange->first;
            addNonPrintData(result, tempBuffer, rawStart, rawEnd, data);
        }
    }

    auto lastOffset = (ranges.last().first + ranges.last().second);
    auto rest = sz - (ranges.last().first + ranges.last().second);

    if (rest) {
        addNonPrintData(result, tempBuffer, lastOffset, sz, data);
    }

    return result;
}
