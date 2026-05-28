#ifndef FOXBENDER_FOXDXFREADER_H
#define FOXBENDER_FOXDXFREADER_H

#include "src/core/dxfdocument.h"

#include <QString>

class FoxDxfReader
{
public:
    struct Result
    {
        bool success = false;
        DxfDocument document;
        QString errorMessage;
    };

    Result loadFile(const QString &filePath) const;
    DxfDocument createDemoDocument() const;
};

#endif // FOXBENDER_FOXDXFREADER_H
