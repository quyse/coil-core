#include "image.hpp"
#include <algorithm>

namespace Coil
{
  std::pair<std::vector<ivec2>, ivec2> Image2DShelfUnion(std::vector<ivec2> const& imageSizes, int32_t maxResultWidth, int32_t border)
  {
    size_t imagesCount = imageSizes.size();

    // sort images by height
    std::vector<size_t> sortedImages(imagesCount);
    for(size_t i = 0; i < imagesCount; ++i)
      sortedImages[i] = i;
    std::sort(sortedImages.begin(), sortedImages.end(), [&](size_t a, size_t b)
    {
      return imageSizes[a].y() < imageSizes[b].y();
    });

    // place images
    int32_t resultWidth = border, resultHeight = border;
    std::vector<ivec2> positions(imagesCount);
    {
      int32_t currentX = border, currentRowHeight = 0;
      for(size_t i = 0; i < imagesCount; ++i)
      {
        auto const& imageSize = imageSizes[sortedImages[i]];
        int32_t width = imageSize.x();
        if(currentX + width + border > maxResultWidth)
        {
          // move to the next row
          resultHeight += currentRowHeight;
          currentRowHeight = 0;
          currentX = border;
        }

        positions[sortedImages[i]] = { currentX, resultHeight };

        currentX += width + border;
        resultWidth = std::max(resultWidth, currentX);
        currentRowHeight = std::max(currentRowHeight, imageSize.y() + border);
      }
      resultHeight += currentRowHeight;
    }

    return
    {
      std::move(positions),
      { resultWidth, resultHeight },
    };
  }
}
