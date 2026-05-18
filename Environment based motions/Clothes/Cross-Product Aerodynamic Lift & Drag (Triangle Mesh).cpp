#include <iostream>
#include <cmath>

struct Vec3 {
    float x, y, z;
    Vec3 operator-(const Vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec3 operator+(const Vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    
    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
    }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const { float l = length(); return l > 0 ? Vec3{x/l, y/l, z/l} : Vec3{0,0,0}; }
    static float dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
};

struct Vertex { Vec3 pos; Vec3 vel; Vec3 force; };

class AerodynamicCloth {
public:
    void applyTrueWindForce(Vertex& v0, Vertex& v1, Vertex& v2, const Vec3& windVelocity) {
        // 1. Calculate Triangle Normal & Area
        Vec3 edge1 = v1.pos - v0.pos;
        Vec3 edge2 = v2.pos - v0.pos;
        Vec3 crossProd = Vec3::cross(edge1, edge2);
        
        Vec3 normal = crossProd.normalized();
        float area = crossProd.length() * 0.5f;

        // 2. Average face velocity
        Vec3 faceVel = (v0.vel + v1.vel + v2.vel) * 0.333f;
        Vec3 relVel = windVelocity - faceVel;

        // 3. Aerodynamic force formula: F = 0.5 * rho * v^2 * C_d * Area * Normal
        float rho = 1.225f; // Air density
        float v_len = relVel.length();
        if (v_len > 0.001f) {
            Vec3 relVelNorm = relVel.normalized();
            float cosTheta = Vec3::dot(relVelNorm, normal);
            
            // Force acts along the surface normal proportional to exposed surface area
            Vec3 force = normal * (0.5f * rho * v_len * v_len * cosTheta * area);
            
            // Distribute force evenly to the 3 vertices of the triangle
            Vec3 splitForce = force * 0.333f;
            v0.force = v0.force + splitForce;
            v1.force = v1.force + splitForce;
            v2.force = v2.force + splitForce;
        }
    }
};
