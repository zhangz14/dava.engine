#include "Classes/Painter/Painter.h"

#include <Render/2D/Systems/BatchDescriptor2D.h>
#include <Render/2D/Systems/RenderSystem2D.h>

#include <Render/Material/NMaterial.h>
#include <Render/2D/Systems/RenderSystem2D.h>
#include <Render/2D/TextBlockGraphicRender.h>
#include <Utils/UTF8Utils.h>

namespace Painting
{
using namespace DAVA;

Painter::Painter()
{
    FilePath fntPath = FilePath("~res:/QuickEd/Fonts/korinna.fnt");
    FilePath texPath = FilePath("~res:/QuickEd/Fonts/korinna.tex");
    font = GraphicFont::Create(fntPath, texPath);
    DVASSERT(font != nullptr);

    if (font->GetFontType() == Font::TYPE_DISTANCE)
    {
        cachedSpread = font->GetSpread();
        fontMaterial = new NMaterial();
        fontMaterial->SetFXName(FastName("~res:/Materials/2d.DistanceFont.material"));
        fontMaterial->SetMaterialName(FastName("DistanceFontMaterial"));
        fontMaterial->AddProperty(FastName("smoothing"), &cachedSpread, rhi::ShaderProp::TYPE_FLOAT1);
        fontMaterial->PreBuildMaterial(RenderSystem2D::RENDER_PASS_NAME);
    }
    else
    {
        fontMaterial = SafeRetain(RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL);
    }

    textureMaterial = SafeRetain(RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL);
}

void Painter::Add(uint32 order, DrawTextParams params)
{
    ApplyParamPos(params);
    Vector<DrawTextParams>& items = drawItems[order].drawTextItems;
    items.push_back(params);
}

void Painter::Add(uint32 order, const DrawLineParams& params)
{
    Vector<DrawLineParams>& items = drawItems[order].drawLineItems;
    items.push_back(params);
}

void Painter::Draw(Window* window)
{
    for (auto& pair : drawItems)
    {
        for (const DrawLineParams& params : pair.second.drawLineItems)
        {
            Draw(params);
        }
        for (const DrawTextParams& params : pair.second.drawTextItems)
        {
            Draw(params);
        }
    }
    drawItems.clear();
}

void Painter::Draw(const DrawLineParams& params)
{
    Matrix3 transformMatrix;
    params.gd.BuildTransformMatrix(transformMatrix);

    Vector2 start = params.startPos * transformMatrix;
    Vector2 end = params.endPos * transformMatrix;
    if (params.type == DrawLineParams::SOLID)
    {
        RenderSystem2D::Instance()->DrawLine(start, end, params.width, params.color);
    }
    else
    {
        Vector2 arrow = end - start;
        float32 length = arrow.Length();
        arrow.Normalize();
        bool dot = true;
        const uint32 dotStep = 3;
        for (float32 i = 0.0f; i < length; i += dotStep)
        {
            Vector2 relativeStart = start + arrow * i;
            Vector2 relativeEnd = start + arrow * (i + dotStep);
            Color color = dot ? params.color : Color::Transparent;
            dot = !dot;
            RenderSystem2D::Instance()->DrawLine(relativeStart, relativeEnd, params.width, color);
        }
    }
}

void Painter::Draw(const DrawTextParams& params)
{
    font->SetSize(params.textSize);
    vertices.resize(4 * params.text.length());

    int32 charactersDrawn = 0;

    font->DrawStringToBuffer(UTF8Utils::EncodeToWideString(params.text), static_cast<int32>(params.gd.scale.x * params.pos.x), static_cast<int32>(params.gd.scale.x * params.pos.y), vertices.data(), charactersDrawn);
    DVASSERT(charactersDrawn == params.text.length());

    uint32 vertexCount = static_cast<uint32>(vertices.size());
    uint32 indexCount = 6 * vertexCount / 4;

    if (font->GetFontType() == Font::TYPE_DISTANCE)
    {
        float32 spread = font->GetSpread();
        if (!FLOAT_EQUAL(cachedSpread, spread))
        {
            cachedSpread = spread;
            fontMaterial->SetPropertyValue(FastName("smoothing"), &cachedSpread);
        }
    }

    Matrix4 rotateMatrix;
    rotateMatrix.BuildRotation(Vector3(0.f, 0.f, -1.0f), params.gd.angle);

    Matrix3 transformMatrix;
    params.gd.BuildTransformMatrix(transformMatrix);

    Matrix4 worldMatrix;
    worldMatrix.BuildTranslation(Vector3(transformMatrix._20, transformMatrix._21, 0.f));

    Matrix4 resultMatrix = rotateMatrix * worldMatrix;

    BatchDescriptor2D batchDescriptor;
    batchDescriptor.singleColor = params.color;
    batchDescriptor.vertexCount = vertexCount;
    batchDescriptor.indexCount = Min(TextBlockGraphicRender::GetSharedIndexBufferCapacity(), indexCount);
    batchDescriptor.vertexPointer = vertices.front().position.data;
    batchDescriptor.vertexStride = TextBlockGraphicRender::TextVerticesDefaultStride;
    batchDescriptor.texCoordPointer[0] = vertices.front().texCoord.data;
    batchDescriptor.texCoordStride = TextBlockGraphicRender::TextVerticesDefaultStride;
    batchDescriptor.indexPointer = TextBlockGraphicRender::GetSharedIndexBuffer();
    batchDescriptor.material = fontMaterial.Get();
    batchDescriptor.textureSetHandle = font->GetTexture()->singleTextureSet;
    batchDescriptor.samplerStateHandle = font->GetTexture()->samplerStateHandle;
    batchDescriptor.worldMatrix = &resultMatrix;

    RenderSystem2D::Instance()->PushBatch(batchDescriptor);
}

void Painter::ApplyParamPos(DrawTextParams& params) const
{
    Font::StringMetrics metrics = font->GetStringMetrics(UTF8Utils::EncodeToWideString(params.text));
    //while we using hard-coded font we need to fix it base line manually
    //DejaVuSans have a very big height which is invalid for digits. So while we use only digits, and font DejaVuSans and GraphicsFont have no GetBaseLine member function - i will change metrics height manually
    Vector2 size = Vector2(metrics.width, metrics.height);

    size /= params.gd.scale;
    params.margin /= params.gd.scale;

    if (params.direction & ALIGN_LEFT)
    {
        params.pos.x -= (size.dx + params.margin.x);
    }
    else if (params.direction & ALIGN_HCENTER)
    {
        params.pos.x -= size.dx / 2.0f;
    }
    else if (params.direction & ALIGN_RIGHT)
    {
        params.pos.x += params.margin.x;
    }
    else
    {
        DVASSERT(false, "horisontal direction must be specified");
    }

    if (params.direction & ALIGN_TOP)
    {
        params.pos.y -= (size.dy + params.margin.y);
    }
    else if (params.direction & ALIGN_VCENTER)
    {
        params.pos.y -= size.dy / 2.0f;
    }
    else if (params.direction & ALIGN_BOTTOM)
    {
        params.pos.y += params.margin.y;
    }
    else
    {
        DVASSERT(false, "vertical direction must be specified");
    }

    //font have a little padding inside it draw rect
    const Vector2 padding = Vector2(2.0f, 2.0f) / params.gd.scale;
    params.pos.x -= padding.x;
    params.pos.y -= padding.y;
}
}
