#include "pch.h"
#include "msCommon.h"

namespace ms {

namespace {

template<class T>
struct ssize_impl
{
    uint32_t operator()(const T&) { return sizeof(T); }
};
template<class T>
struct ssize_impl<RawVector<T>>
{
    uint32_t operator()(const RawVector<T>& v) { return uint32_t(4 + sizeof(T) * v.size()); }
};
template<>
struct ssize_impl<std::string>
{
    uint32_t operator()(const std::string& v) { return uint32_t(4 + v.size()); }
};

template<class T>
inline uint32_t ssize(const T& v) { return ssize_impl<T>()(v); }


template<class T>
struct write_impl
{
    void operator()(std::ostream& os, const T& v)
    {
        os.write((const char*)&v, sizeof(T));
    }
};
template<class T>
struct write_impl<RawVector<T>>
{
    void operator()(std::ostream& os, const RawVector<T>& v)
    {
        auto size = (uint32_t)v.size();
        os.write((const char*)&size, 4);
        os.write((const char*)v.data(), sizeof(T) * size);
    }
};
template<>
struct write_impl<std::string>
{
    void operator()(std::ostream& os, const std::string& v)
    {
        auto size = (uint32_t)v.size();
        os.write((const char*)&size, 4);
        os.write((const char*)v.data(), size);
    }
};
template<class T>
inline void write(std::ostream& os, const T& v) { return write_impl<T>()(os, v); }



template<class T>
struct read_impl
{
    void operator()(std::istream& is, T& v)
    {
        is.read((char*)&v, sizeof(T));
    }
};
template<class T>
struct read_impl<RawVector<T>>
{
    void operator()(std::istream& is, RawVector<T>& v)
    {
        uint32_t size = 0;
        is.read((char*)&size, 4);
        v.resize(size);
        is.read((char*)v.data(), sizeof(T) * size);
    }
};
template<>
struct read_impl<std::string>
{
    void operator()(std::istream& is, std::string& v)
    {
        uint32_t size = 0;
        is.read((char*)&size, 4);
        v.resize(size);
        is.read((char*)v.data(), size);
    }
};
template<class T>
inline void read(std::istream& is, T& v) { return read_impl<T>()(is, v); }

} // namespace




MessageData::~MessageData()
{
}

GetData::GetData()
{
}
uint32_t GetData::getSerializeSize() const
{
    uint32_t ret = 0;
    ret += ssize(flags);
    ret += ssize(scale);
    return ret;
}
void GetData::serialize(std::ostream& os) const
{
    write(os, flags);
    write(os, scale);
}
void GetData::deserialize(std::istream& is)
{
    read(is, flags);
    read(is, scale);
}


DeleteData::DeleteData()
{
}
uint32_t DeleteData::getSerializeSize() const
{
    uint32_t ret = 0;
    ret += ssize(path);
    return ret;
}
void DeleteData::serialize(std::ostream& os) const
{
    write(os, path);
}
void DeleteData::deserialize(std::istream& is)
{
    read(is, path);
}


uint32_t ClientSpecificData::getSerializeSize() const
{
    uint32_t ret = ssize(type);
    switch (type) {
    case Type::Metasequia:
        ret += ssize(mq);
        break;
    }
    return ret;
}
void ClientSpecificData::serialize(std::ostream& os) const
{
    write(os, type);
    switch (type) {
    case Type::Metasequia:
        write(os, mq);
        break;
    }
}
void ClientSpecificData::deserialize(std::istream& is)
{
    read(is, type);
    switch (type) {
    case Type::Metasequia:
        read(is, mq);
        break;
    }
}


#define EachArray(Body) Body(points) Body(normals) Body(tangents) Body(uv) Body(counts) Body(indices)

MeshData::MeshData()
{
}

MeshData::MeshData(const MeshDataCS & cs)
{
    id = cs.id;
    path = cs.path ? cs.path : "";
    flags = cs.flags;
    if (flags.has_points && cs.points) points.assign(cs.points, cs.points + cs.num_points);
    if (flags.has_normals && cs.normals) normals.assign(cs.normals, cs.normals + cs.num_points);
    if (flags.has_tangents && cs.tangents) tangents.assign(cs.tangents, cs.tangents + cs.num_points);
    if (flags.has_uv && cs.uv) uv.assign(cs.uv, cs.uv + cs.num_points);
    if (flags.has_indices && cs.indices) indices.assign(cs.indices, cs.indices + cs.num_indices);
    if (flags.has_transform) transform = cs.transform;
}

void MeshData::clear()
{
    id = 0;
    path.clear();
    flags = {0};
#define Body(A) A.clear();
    EachArray(Body);
#undef Body
    transform = Transform();
    csd = ClientSpecificData();
    submeshes.clear();
}

uint32_t MeshData::getSerializeSize() const
{
    uint32_t ret = 0;
    ret += ssize(id);
    ret += ssize(path);
    ret += ssize(flags);
#define Body(A) if(flags.has_##A) ret += ssize(A);
    EachArray(Body);
#undef Body
    if (flags.has_transform) ret += ssize(transform);
    ret += csd.getSerializeSize();
    return ret;
}

void MeshData::serialize(std::ostream& os) const
{
    write(os, id);
    write(os, path);
    write(os, flags);
#define Body(A) if(flags.has_##A) write(os, A);
    EachArray(Body);
#undef Body
    if (flags.has_transform) write(os, transform);
    csd.serialize(os);
}

void MeshData::deserialize(std::istream& is)
{
    read(is, id);
    read(is, path);
    read(is, flags);
#define Body(A) if(flags.has_##A) read(is, A);
    EachArray(Body);
#undef Body
    if (flags.has_transform) read(is, transform);
    csd.deserialize(is);
}

const char* MeshData::getName() const
{
    size_t name_pos = path.find_last_of('/');
    if (name_pos != std::string::npos) { ++name_pos; }
    else { name_pos = 0; }
    return path.c_str() + name_pos;
}

void MeshData::swap(MeshData & v)
{
    std::swap(id, v.id);
    path.swap(v.path);
    std::swap(flags, v.flags);
#define Body(A) A.swap(v.A);
    EachArray(Body);
#undef Body
    std::swap(transform, v.transform);
    std::swap(csd, v.csd);

    std::swap(submeshes, v.submeshes);
}

void MeshData::refine(const MeshRefineSettings &settings)
{
    if (settings.flags.apply_local2world) {
        applyTransform(transform.local2world);
    }
    if (settings.flags.apply_world2local) {
        applyTransform(transform.world2local);
        flags.has_transform = 0;
    }
    if (settings.scale != 1.0f) {
        mu::Scale(points.data(), settings.scale, points.size());
    }
    if (settings.flags.swap_handedness) {
        mu::InvertX(points.data(), points.size());
    }

    RawVector<int> offsets;
    int num_indices = 0;
    int num_indices_tri = 0;
    if (counts.empty()) {
        // assume all faces are triangle
        num_indices = num_indices_tri = (int)indices.size();
        int num_faces = num_indices / 3;
        counts.resize(num_faces, 3);
        offsets.resize(num_faces);
        for (int i = 0; i < num_faces; ++i) {
            offsets[i] = i * 3;
        }
    }
    else {
        mu::CountIndices(counts, offsets, num_indices, num_indices_tri);
    }

    // normals
    bool gen_normals = settings.flags.gen_normals && normals.empty();
    if (gen_normals) {
        bool do_smoothing =
            csd.type == ClientSpecificData::Type::Metasequia && (csd.mq.smooth_angle >= 0.0f && csd.mq.smooth_angle < 360.0f);
        if (do_smoothing) {
            normals.resize(indices.size());
            mu::GenerateNormalsWithThreshold(normals, points, counts, offsets, indices, csd.mq.smooth_angle);
        }
        else {
            normals.resize(points.size());
            mu::GenerateNormals(normals, points, counts, offsets, indices);
        }
    }

    // flatten
    bool flatten = false;
    if ((int)points.size() > settings.split_unit ||
        (int)normals.size() == num_indices ||
        (int)uv.size() == num_indices ||
        (int)tangents.size() == num_indices)
    {
        {
            decltype(points) flattened;
            mu::CopyWithIndices(flattened, points, indices, 0, num_indices);
            points.swap(flattened);
        }
        if (!normals.empty() && (int)normals.size() != num_indices) {
            decltype(normals) flattened;
            mu::CopyWithIndices(flattened, normals, indices, 0, num_indices);
            normals.swap(flattened);
        }
        if (!uv.empty() && (int)uv.size() != num_indices) {
            decltype(uv) flattened;
            mu::CopyWithIndices(flattened, uv, indices, 0, num_indices);
            uv.swap(flattened);
        }
        if (!tangents.empty() && (int)tangents.size() != num_indices) {
            decltype(tangents) flattened;
            mu::CopyWithIndices(flattened, tangents, indices, 0, num_indices);
            tangents.swap(flattened);
        }
        std::iota(indices.begin(), indices.end(), 0);
        flatten = true;
    }

    // tangents
    bool gen_tangents = settings.flags.gen_tangents && tangents.empty() && !normals.empty() && !uv.empty();
    if (gen_tangents) {
        tangents.resize(points.size());
        mu::GenerateTangents(tangents, points, normals, uv, counts, offsets, indices);
    }

    // split & triangulate
    bool split = settings.flags.split && points.size() > settings.split_unit;
    if (split) {
        submeshes.clear();
        indices.resize(num_indices_tri);
        int *sub_indices = indices.data();
        mu::Split(counts, settings.split_unit, [&](int face_begin, int num_faces, int num_vertices, int num_indices_triangulated) {
            auto sub = Submesh();

            sub.indices.reset(sub_indices, num_indices_triangulated);
            mu::Triangulate(sub.indices, IntrusiveArray<int>(&counts[face_begin], num_faces), settings.flags.swap_faces);
            sub_indices += num_indices_triangulated;

            size_t ibegin = offsets[face_begin];
            if (!points.empty()) {
                sub.points.reset(&points[ibegin], num_vertices);
            }
            if (!normals.empty()) {
                sub.normals.reset(&normals[ibegin], num_vertices);
            }
            if (!uv.empty()) {
                sub.uv.reset(&uv[ibegin], num_vertices);
            }
            if (!tangents.empty()) {
                sub.tangents.reset(&tangents[ibegin], num_vertices);
            }

            submeshes.push_back(sub);
        });
    }
    else if(settings.flags.triangulate) {
        decltype(indices) indices_triangulated(num_indices_tri);
        if (flatten) {
            mu::Triangulate(indices_triangulated, counts, settings.flags.swap_faces);
        }
        else {
            mu::TriangulateWithIndices(indices_triangulated, counts, indices, settings.flags.swap_faces);
        }
        indices.swap(indices_triangulated);
    }
}


void MeshData::applyTransform(const float4x4& m)
{
    for (auto& v : points) { v = applyTRS(m, v); }
    for (auto& v : normals) { v = m * v; }
    mu::Normalize(normals.data(), normals.size());
}



GetDataCS::GetDataCS(const GetData& v)
{
    flags = v.flags;
}

DeleteDataCS::DeleteDataCS(const DeleteData & v)
{
    obj_path = v.path.c_str();
}


MeshDataCS::MeshDataCS(const MeshData & v)
{
    id          = v.id;
    flags       = v.flags;
    cpp         = const_cast<MeshData*>(&v);
    path        = v.path.c_str();
    points      = (float3*)v.points.data();
    normals     = (float3*)v.normals.data();
    tangents    = (float4*)v.tangents.data();
    uv          = (float2*)v.uv.data();
    indices     = (int*)v.indices.data();
    num_points  = (int)v.points.size();
    num_indices = (int)v.indices.size();
    num_submeshes = (int)v.submeshes.size();
}

SubmeshDataCS::SubmeshDataCS()
{
}

SubmeshDataCS::SubmeshDataCS(const Submesh & v)
{
    points      = (float3*)v.points.data();
    normals     = (float3*)v.normals.data();
    tangents    = (float4*)v.tangents.data();
    uv          = (float2*)v.uv.data();
    indices     = (int*)v.indices.data();
    num_points  = (int)v.points.size();
    num_indices = (int)v.indices.size();
}

} // namespace ms