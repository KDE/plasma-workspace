#include "shades.h"

#include "colorutils.h"

namespace Shades
{
std::array<QRgb, 12> of(double hue, double chroma)
{
    std::array<QRgb, 12> shades;
    // At tone 90 and above, blue and yellow hues can reach a much higher chroma.
    // To preserve a consistent appearance across all hues, use a maximum chroma of 40.
    shades[0] = ColorUtils::CAMToColor(hue, std::min(40.0, chroma), 99);
    shades[1] = ColorUtils::CAMToColor(hue, std::min(40.0, chroma), 95);
    for (int i = 2; i < 12; i++) {
        const double lStar = i == 6 ? MIDDLE_LSTAR : 100 - 10 * (i - 1);
        shades[i] = ColorUtils::CAMToColor(hue, chroma, lStar);
    }
    return shades;
}

}
