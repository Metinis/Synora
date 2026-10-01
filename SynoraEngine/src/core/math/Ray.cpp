#include <SynoraEngine/core/math/Ray.h>

#include <SynoraEngine/project/assets/ModelData.h>

namespace SYN {
Ray Ray::from(glm::vec3 position, glm::vec3 direction) {
    return {position, glm::normalize(direction)};
}

glm::vec3 Ray::positionAt(float t) const { return position + direction * t; }

Ray Ray::screenToWorld(glm::mat4 viewProjection, glm::vec3 cameraPosition,
                       glm::vec2 screenPos, uint32_t width, uint32_t height) {
    glm::vec3 ndc;
    ndc.x = (2.0f * screenPos.x) / (float)width - 1.0f;
    ndc.y = 1.0f - (2.0f * screenPos.y) / (float)height;
    ndc.z = 1.0f;

    glm::mat4 invCam = glm::inverse(viewProjection);

    glm::vec4 nearPoint = invCam * glm::vec4(ndc.x, ndc.y, 0.0f, 1.0f);
    glm::vec4 farPoint = invCam * glm::vec4(ndc.x, ndc.y, ndc.z, 1.0f);

    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    glm::vec3 dir = glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint));

    return Ray::from(cameraPosition, dir);
}

std::optional<Ray::Hit> Ray::collidesWithAABB(AABB aabb) const {
    float maxVal = std::numeric_limits<float>().max();

    float tMin = -maxVal;
    float tMax = maxVal;

    float epsilon = std::numeric_limits<float>().epsilon();

    uint32_t entryAxis = 0;
    uint32_t exitAxis = 0;
    for (uint32_t i = 0; i < 3; ++i) {
        if (glm::abs(direction[i]) < epsilon) {
            if (position[i] < aabb.min[i] || position[i] > aabb.max[i])
                return std::nullopt;
            continue;
        }

        float dirInv = 1.0f / direction[i];
        float t0 = (aabb.min[i] - position[i]) * dirInv;
        float t1 = (aabb.max[i] - position[i]) * dirInv;

        if (t0 > t1)
            std::swap(t0, t1);

        if (t0 > tMin) {
            entryAxis = i;
            tMin = t0;
        }

        if (t1 < tMax) {
            exitAxis = i;
            tMax = t1;
        }

        if (tMin > tMax)
            return std::nullopt;
    }

    bool isInside = false;
    if (tMin < 0.0f) {
        tMin = tMax;
        isInside = true;
        if (tMin < 0.0f)
            return std::nullopt;
    }

    Hit info;
    info.distance = tMin;
    info.position = positionAt(info.distance);
    info.normal = glm::vec3(0.0f);
    uint32_t hitAxis = isInside ? exitAxis : entryAxis;
    info.normal[hitAxis] = isInside ? glm::sign(direction[hitAxis])
                                    : -glm::sign(direction[hitAxis]);

    return info;
}

std::optional<Ray::Hit> Ray::collidesWithSphere(Sphere sphere) const {
    glm::vec3 L = position - sphere.center;
    float a = glm::dot(direction, direction);
    float b = 2.0f * glm::dot(direction, L);
    float c = glm::dot(L, L) - (sphere.radius * sphere.radius);

    float tMin, tMax;

    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0.0f)
        return std::nullopt;
    else if (discriminant == 0.0f)
        tMin = tMax = -0.5f * b / a;
    else {
        float q = -0.5 * (b + glm::sign(b) * sqrtf(discriminant));
        if (q == 0.0f) {
            float sqrtDisc = std::sqrt(discriminant);
            tMin = -b + sqrtDisc * -0.5f;
            tMax = -b - sqrtDisc * -0.5f;
        } else {
            tMin = q / a;
            tMax = c / q;
        }
    }

    if (tMin > tMax)
        std::swap(tMin, tMax);
    if (tMin < 0.0f) {
        tMin = tMax;
        if (tMin < 0.0f)
            return std::nullopt;
    }

    Hit info;
    info.distance = tMin;
    info.position = positionAt(info.distance);
    info.normal = glm::normalize(info.position - sphere.center);

    return info;
}

std::optional<Ray::Hit> Ray::collidesWithPlane(Plane plane) const {
    glm::vec3 normal = glm::normalize(glm::vec3(plane.a, plane.b, plane.c));

    float denom = glm::dot(normal, direction);

    if (glm::abs(denom) < std::numeric_limits<float>().epsilon()) {
        return std::nullopt;
    }

    float tMin = (-plane.d - glm::dot(normal, position)) / denom;

    if (tMin <= 0.0f)
        return std::nullopt;

    Hit info;
    info.distance = tMin;
    info.position = positionAt(info.distance);
    info.normal = denom > 0.0f ? -normal : normal;

    return info;
}

std::optional<Ray::Hit> Ray::collidesWithTriangle(glm::vec3 a, glm::vec3 b,
                                                  glm::vec3 c) {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ao = position - a;

    // For cramer's rule this can't be normalized
    // yet
    glm::vec3 normal = glm::cross(ab, ac);

    float d = glm::dot(-direction, normal);
    float nLength2 = glm::dot(normal, normal);
    constexpr float epsilonSqr = 1e-4;
    if (d * d <= epsilonSqr * nLength2)
        return std::nullopt;

    float s = glm::sign(d);
    d = glm::abs(d);

    float t = s * glm::dot(ao, normal);
    if (t < 0.0f)
        return std::nullopt;

    glm::vec3 e = glm::cross(-direction, ao);
    float v = s * glm::dot(ac, e);
    if (v < 0.0f || v > d)
        return std::nullopt;
    float w = s * -glm::dot(ab, e);
    if (w < 0.0f || v + w > d)
        return std::nullopt;

    float invD = 1.0f / d;

    t *= invD;

    Ray::Hit info;
    info.distance = t;
    info.normal = s * normal * glm::inversesqrt(nLength2);
    info.position = position + direction * info.distance;

    return info;
}

std::optional<Ray::Hit> Ray::collidesWithMesh(const MeshData *mesh,
                                              glm::mat4 world) {
    if (mesh == nullptr)
        return std::nullopt;

    glm::mat4 modelMatrix = world * mesh->localTransform;
    glm::mat4 invModel = glm::inverse(modelMatrix);
    glm::vec3 modelOrigin = invModel * glm::vec4(position, 1.0f);
    glm::vec3 modelDir = glm::normalize(invModel * glm::vec4(direction, 0.0f));
    Ray modelRay = Ray::from(modelOrigin, modelDir);

    std::optional<Ray::Hit> result;
    for (uint32_t i = 0; i < mesh->indices.size(); i += 3) {
        uint32_t aIndex = mesh->indices.at(i);
        uint32_t bIndex = mesh->indices.at(i + 1);
        uint32_t cIndex = mesh->indices.at(i + 2);

        Vertex a = mesh->vertices.at(aIndex);
        Vertex b = mesh->vertices.at(bIndex);
        Vertex c = mesh->vertices.at(cIndex);

        glm::vec3 aPos = a.position;
        glm::vec3 bPos = b.position;
        glm::vec3 cPos = c.position;

        if (auto hit = modelRay.collidesWithTriangle(aPos, bPos, cPos);
            hit.has_value()) {
            if (!result.has_value()) {
                result = hit;
                continue;
            }
            float currentDistance = result.value().distance;
            if (currentDistance > hit.value().distance) {
                result = hit;
            }
        }
    }

    if (!result.has_value())
        return std::nullopt;

    Ray::Hit modelHitInfo = result.value();
    modelHitInfo.position =
        modelMatrix * glm::vec4(modelHitInfo.position, 1.0f);

    glm::mat3 normalMatrix =
        glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
    modelHitInfo.normal = glm::normalize(normalMatrix * modelHitInfo.normal);
    modelHitInfo.distance = glm::length(modelHitInfo.position - position);

    return modelHitInfo;
}

} // namespace SYN
