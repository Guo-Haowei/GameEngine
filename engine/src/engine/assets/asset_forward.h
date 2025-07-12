#pragma once

namespace my {

struct IAsset;
struct BufferAsset;
struct TextAsset;
struct ImageAsset;

using AssetRef = std::shared_ptr<IAsset>;
using BufferRef = std::shared_ptr<BufferAsset>;
using TextRef = std::shared_ptr<TextAsset>;
using ImageRef = std::shared_ptr<ImageAsset>;

}
