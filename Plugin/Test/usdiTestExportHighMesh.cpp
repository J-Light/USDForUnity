#include "pch.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
#include <tbb/tbb.h>
#include "Mesh.h"

void TestExportHighMesh(const char *filename, int frame_count)
{
    auto *ctx = usdiCreateContext();

    usdi::ExportSettings settings;
    settings.instanceable_by_default = true;
    usdiSetExportSettings(ctx, &settings);

    usdiCreateStage(ctx, filename);
    auto *root = usdiGetRoot(ctx);

    auto *xf = usdiCreateXform(ctx, root, "WaveMeshRoot");
    usdiPrimSetInstanceable(xf, true);
    {
        usdi::XformData data;
        usdiXformWriteSample(xf, &data);
    }

    auto *mesh = usdiCreateMesh(ctx, xf, "WaveMesh");
    {
        std::vector<std::vector<int>> counts(frame_count);
        std::vector<std::vector<int>> indices(frame_count);
        std::vector<std::vector<float3>> points(frame_count);
        std::vector<std::vector<float2>> uv(frame_count);
        std::vector<std::vector<float4>> colors(frame_count);

        usdi::Time t = 0.0;

        tbb::parallel_for(0, frame_count, [frame_count, &counts, &indices, &points, &uv, &colors](int i) {
            usdi::Time t = i;
            int resolution = 8;
            if (i < 30)      { resolution = 8; }
            else if (i < 60) { resolution = 16; }
            else if (i < 90) { resolution = 32; }
            else if (i < 120) { resolution = 64; }
            else if (i < 150) { resolution = 128; }
            else { resolution = 256; }
            GenerateWaveMesh(counts[i], indices[i], points[i], uv[i], 1.0f, 0.5f, resolution, (360.0 * 5 * DegToRad / frame_count) * i);

            auto& color = colors[i];
            color.resize(points[i].size());
            for (size_t ci = 0; ci < color.size(); ++ci) {
                color[ci] = {
                    std::sin((float)i * DegToRad * 2.1f) * 0.5f + 0.5f,
                    std::cos((float)i * DegToRad * 6.8f) * 0.5f + 0.5f,
                    std::sin((float)i * DegToRad * 11.7f) * 0.5f + 0.5f,
                    1.0f
                };
            }
        });
        for (int i = 0; i < frame_count; ++i) {
            auto& vertices = points[i];

            usdi::Time t = i;
            usdi::MeshData data;
            data.faces = counts[i].data();
            data.face_count = counts[i].size();
            data.indices = indices[i].data();
            data.index_count = indices[i].size();
            data.points = points[i].data();
            data.vertex_count = points[i].size();
            data.uv0 = uv[i].data();
            data.colors = colors[i].data();
            usdiMeshWriteSample(mesh, &data, t);
        }
    }

    {
        auto *ref1 = usdiCreateXform(ctx, root, "WaveMeshRef1");
        usdi::XformData data;
        data.position.x = 1.5f;
        usdiXformWriteSample(ref1, &data);

        auto *ref = usdiCreateOverride(ctx, "/WaveMeshRef1/Ref");
        usdiPrimAddReference(ref, nullptr, "/WaveMeshRoot");
    }
    {
        auto *ref2 = usdiCreateXform(ctx, root, "WaveMeshRef2");
        usdi::XformData data;
        data.position.x = -1.5f;
        usdiXformWriteSample(ref2, &data);

        auto *ref = usdiCreateOverride(ctx, "/WaveMeshRef2/Ref");
        usdiPrimAddReference(ref, nullptr, "/WaveMeshRoot");
    }

    usdiSave(ctx);
    usdiDestroyContext(ctx);
}
