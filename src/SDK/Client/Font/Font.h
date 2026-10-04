#pragma once

class Font {
private:
    virtual void destructor();
    virtual void reserved1();
    virtual void reserved2();
    virtual void reserved3();
    virtual void reserved4();
    virtual void reserved5();

public:
    float getLineLength(const std::string_view& str, float fontSize, bool showColorSymbol = false);
    float getLineHeight();
};
