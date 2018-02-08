#include "pch.h"
#include "usdiInternal.h"
#include "usdiSchema.h"
#include "usdiXform.h"
#include "usdiContext.h"
#include "usdiAttribute.h"

//#define usdiSerializeRotationAsEuler

namespace usdi {

static quatf EulerToQuaternion(const float3& euler, UsdGeomXformOp::Type order)
{
    quatf qX = rotateX(euler.x);
    quatf qY = rotateY(euler.y);
    quatf qZ = rotateZ(euler.z);

    switch (order) {
    case UsdGeomXformOp::TypeRotateXYZ: return (qZ * qY) * qX;
    case UsdGeomXformOp::TypeRotateXZY: return (qY * qZ) * qX;
    case UsdGeomXformOp::TypeRotateYXZ: return (qZ * qX) * qY;
    case UsdGeomXformOp::TypeRotateYZX: return (qX * qZ) * qY;
    case UsdGeomXformOp::TypeRotateZXY: return (qY * qX) * qZ;
    case UsdGeomXformOp::TypeRotateZYX: return (qX * qY) * qZ;
    default:
        usdiLogError("EulerToQuaternion(): unknown operation\n");
        return quatf::identity();
    }
}

static float Clamp(float v, float vmin, float vmax) { return std::min<float>(std::max<float>(v, vmin), vmax); }
static float Saturate(float v) { return Clamp(v, -1.0f, 1.0f); }

float3 QuaternionToEulerZXY(const quatf& q)
{
    float d[] = {
        q.x*q.x, q.x*q.y, q.x*q.z, q.x*q.w,
        q.y*q.y, q.y*q.z, q.y*q.w,
        q.z*q.z, q.z*q.w,
        q.w*q.w
    };

    float v0 = d[5] - d[3];
    float v1 = 2.0f*(d[1] + d[8]);
    float v2 = d[4] - d[7] - d[0] + d[9];
    float v3 = -1.0f;
    float v4 = 2.0f * v0;

    const float SINGULARITY_CUTOFF = 0.499999f;
    if (std::abs(v0) < SINGULARITY_CUTOFF)
    {
        float v5 = 2.0f * (d[2] + d[6]);
        float v6 = d[7] - d[0] - d[4] + d[9];

        return{
            v3 * std::asin(Saturate(v4)),
            std::atan2(v5, v6),
            std::atan2(v1, v2)
        };
    }
    else //x == yzy z == 0
    {
        float a = d[1] + d[8];
        float b =-d[5] + d[3];
        float c = d[1] - d[8];
        float e = d[5] + d[3];

        float v5 = a*e + b*c;
        float v6 = b*e - a*c;

        return{
            v3 * std::asin(Saturate(v4)),
            std::atan2(v5, v6),
            0.0f
        };
    }
}


RegisterSchemaHandler(Xform)

Xform::Xform(Context *ctx, Schema *parent, const UsdPrim& prim)
    : super(ctx, parent, prim)
    , m_xf(prim)
{
    usdiLogTrace("Xform::Xform(): %s\n", getPath());
    if (!m_xf) { usdiLogError("Xform::Xform(): m_xf is invalid\n"); }
    interpretXformOps();
}

Xform::Xform(Context *ctx, Schema *parent, const char *name, const char *type)
    : super(ctx, parent, name, type)
    , m_xf(m_prim)
{
    usdiLogTrace("Xform::Xform(): %s\n", getPath());
    if (!m_xf) { usdiLogError("Xform::Xform(): m_xf is invalid\n"); }
}

Xform::~Xform()
{
    usdiLogTrace("Xform::~Xform(): %s\n", getPath());
}

void Xform::interpretXformOps()
{
    bool reset_stack = false;
    m_read_ops = m_xf.GetOrderedXformOps(&reset_stack);

    const int T = 1;
    const int R = 2;
    const int S = 3;
    const int Other = 4;

    std::vector<int> optypes;
    for (auto& op : m_read_ops) {
        switch (op.GetOpType()) {
        case UsdGeomXformOp::TypeTranslate: optypes.push_back(T); break;
        case UsdGeomXformOp::TypeScale: optypes.push_back(S); break;
        case UsdGeomXformOp::TypeRotateX:   // 
        case UsdGeomXformOp::TypeRotateY:   // 
        case UsdGeomXformOp::TypeRotateZ:   // 
        case UsdGeomXformOp::TypeRotateXYZ: // 
        case UsdGeomXformOp::TypeRotateXZY: // 
        case UsdGeomXformOp::TypeRotateYXZ: // 
        case UsdGeomXformOp::TypeRotateYZX: // 
        case UsdGeomXformOp::TypeRotateZXY: // 
        case UsdGeomXformOp::TypeRotateZYX: // fall through
        case UsdGeomXformOp::TypeOrient:    optypes.push_back(R); break;
        default: optypes.push_back(Other); break;
        }
    }

    // determine is this Xform is TRS or not.
    // if operation order matches /T*R*S*/, it is TRS
    // e.g: Translate, RotateZ, rotateX, rotateY, Scale -> TRRRS -> TRS
    optypes.erase(std::unique(optypes.begin(), optypes.end()), optypes.end());
    if (optypes.empty() ||
        optypes == std::vector<int>{T} ||
        optypes == std::vector<int>{T, R} ||
        optypes == std::vector<int>{T, S} ||
        optypes == std::vector<int>{T, R, S} ||
        optypes == std::vector<int>{R} ||
        optypes == std::vector<int>{R, S} ||
        optypes == std::vector<int>{S}
    )
    {
        m_summary.type = XformSummary::Type::TRS;
    }
    else {
        m_summary.type = XformSummary::Type::Matrix;
    }
}

const XformSummary& Xform::getSummary() const
{
    if (m_summary_needs_update) {
        getTimeRange(m_summary.start, m_summary.end);

        m_summary_needs_update = false;
    }
    return m_summary;
}

void Xform::updateSample(Time t_)
{
    super::updateSample(t_);
    if (m_update_flag.bits == 0) {
        m_sample.flags = (m_sample.flags & ~(int)XformData::Flags::UpdatedMask);
        return;
    }
    if (m_update_flag.variant_set_changed) { m_summary_needs_update = true; }

    auto t = UsdTimeCode(t_);
    const auto& conf = getImportSettings();

    auto& sample = m_sample;
    auto prev = sample;

    if (m_summary.type == XformSummary::Type::TRS) {
        auto translate  = float3::zero();
        auto scale      = float3::one();
        auto rotation   = quatf::identity();

        for (auto& op : m_read_ops) {
            switch (op.GetOpType()) {
            case UsdGeomXformOp::TypeTranslate:
            {
                float3 tmp;
                op.GetAs((GfVec3f*)&tmp, t);
                translate += tmp;
                break;
            }
            case UsdGeomXformOp::TypeScale:
            {
                float3 tmp;
                op.GetAs((GfVec3f*)&tmp, t);
                scale *= tmp;
                break;
            }
            case UsdGeomXformOp::TypeOrient:
            {
                quatf tmp;
                op.GetAs((GfQuatf*)&tmp, t);
                rotation *= tmp;
                break;
            }
            case UsdGeomXformOp::TypeRotateX:
            {
                float angle;
                op.GetAs(&angle, t);
                rotation *= rotateX(angle * Deg2Rad);
                break;
            }
            case UsdGeomXformOp::TypeRotateY:
            {
                float angle;
                op.GetAs(&angle, t);
                rotation *= rotateY(angle * Deg2Rad);
                break;
            }
            case UsdGeomXformOp::TypeRotateZ:
            {
                float angle;
                op.GetAs(&angle, t);
                rotation *= rotateZ(angle * Deg2Rad);
                break;
            }
            case UsdGeomXformOp::TypeRotateXYZ: // 
            case UsdGeomXformOp::TypeRotateXZY: // 
            case UsdGeomXformOp::TypeRotateYXZ: // 
            case UsdGeomXformOp::TypeRotateYZX: // 
            case UsdGeomXformOp::TypeRotateZXY: // 
            case UsdGeomXformOp::TypeRotateZYX: // fall through
            {
                float3 euler;
                op.GetAs((GfVec3f*)&euler, t);
                rotation *= EulerToQuaternion(euler * Deg2Rad, op.GetOpType());
                break;
            }
            default:
                break;
            }
        }

        if (conf.swap_handedness) {
            translate.x *= -1.0f;
            rotation = swap_handedness(sample.rotation);
        }
        sample.position = translate;
        sample.rotation = rotation;
        sample.scale = scale;
    }
    else {
        GfMatrix4d result;
        result.SetIdentity();
        for (auto& op : m_read_ops) {
            auto m = op.GetOpTransform(t);
            result = m * result;
        }

        GfTransform gft;
        gft.SetMatrix(result);

        (GfMatrix4f&)sample.transform = GfMatrix4f(result);
        (GfVec3f&)sample.position = GfVec3f(gft.GetTranslation());
        (GfQuatf&)sample.rotation = GfQuatf(gft.GetRotation().GetQuat());
        (GfVec3f&)sample.scale = GfVec3f(gft.GetScale());
    }

    int update_flags = 0;
    if (!near_equal(prev.position, sample.position)) {
        update_flags |= (int)XformData::Flags::UpdatedPosition;
    }
    if (!near_equal(prev.rotation, sample.rotation)) {
        update_flags |= (int)XformData::Flags::UpdatedRotation;
    }
    if (!near_equal(prev.scale, sample.scale)) {
        update_flags |= (int)XformData::Flags::UpdatedScale;
    }
    sample.flags = (sample.flags & ~(int)XformData::Flags::UpdatedMask) | update_flags;
}

bool Xform::readSample(XformData& dst, Time t)
{
    if (t != m_time_prev) { updateSample(t); }

    dst = m_sample;
    return true;
}


bool Xform::writeSample(const XformData& src_, Time t_)
{
    auto t = UsdTimeCode(t_);
    const auto& conf = getExportSettings();
    XformData src = src_;

    if (m_write_ops.empty()) {
        m_write_ops.push_back(m_xf.AddTranslateOp(UsdGeomXformOp::PrecisionFloat));
        m_write_ops.push_back(m_xf.AddOrientOp(UsdGeomXformOp::PrecisionFloat));
        //m_write_ops.push_back(m_xf.AddRotateZXYOp(UsdGeomXformOp::PrecisionFloat));
        m_write_ops.push_back(m_xf.AddScaleOp(UsdGeomXformOp::PrecisionFloat));
    }

    if (conf.swap_handedness) {
        src.position.x *= -1.0f;
        src.rotation = swap_handedness(src.rotation);
    }

    m_write_ops[0].Set((const GfVec3f&)src.position, t);
    m_write_ops[1].Set((const GfQuatf&)src.rotation, t);
    //m_write_ops[1].Set((const GfVec3f&)src.rotation_eular, t);
    m_write_ops[2].Set((const GfVec3f&)src.scale, t);
    return true;
}

int Xform::eachSample(const SampleCallback & cb)
{
    if (getSummary().type == XformSummary::Type::TRS) {
        // gather time samples
        std::map<Time, int> times;
        eachAttribute([&times](Attribute *a) {
            if (strncmp(a->getName(), "xformOp:translate", 17) == 0) {
                auto ts = a->getTimeSamples();
                int flag = (int)XformData::Flags::UpdatedPosition;
                if (ts.empty()) {
                    times[usdiDefaultTime()] |= flag;
                }
                else
                {
                    for (auto& t : ts) { times[t] |= flag; }
                }
            }
            else if (strncmp(a->getName(), "xformOp:scale", 13) == 0) {
                auto ts = a->getTimeSamples();
                int flag = (int)XformData::Flags::UpdatedScale;
                if (ts.empty()) {
                    times[usdiDefaultTime()] |= flag;
                }
                else {
                    for (auto& t : ts) { times[t] |= flag; }
                }
            }
            else if (strncmp(a->getName(), "xformOp:rotate", 14) == 0 || strncmp(a->getName(), "xformOp:orient", 14) == 0) {
                auto ts = a->getTimeSamples();
                int flag = (int)XformData::Flags::UpdatedRotation;
                if (ts.empty()) {
                    times[usdiDefaultTime()] |= flag;
                }
                else {
                    for (auto& t : ts) { times[t] |= flag; }
                }
            }
        });

        XformData data;
        for (const auto& t : times) {
            readSample(data, t.first);
            data.flags = t.second;;
            cb(data, t.first);
        }
        return (int)times.size();
    }
    else {
        // gather time samples
        std::map<Time, int> times;
        eachAttribute([&times](Attribute *a) {
            if (strncmp(a->getName(), "xformOp:", 8) == 0) {
                auto ts = a->getTimeSamples();
                if (ts.empty()) {
                    times[usdiDefaultTime()] = 0;
                }
                else {
                    for (auto& t : ts) { times[t] = 0; }
                }
            }
        });

        XformData data;
        for (const auto& t : times) {
            readSample(data, t.first);
            cb(data, t.first);
        }
        return (int)times.size();
    }
}

} // namespace usdi
