/*
 * ============================================================
 *  CHARACTER ARM & HAND ANIMATION SYSTEM
 *  Covers: Shoulder, Elbow, Wrist, Palm, Fingers (with joints)
 *  Uses: Forward Kinematics + Keyframe Interpolation
 * ============================================================
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <map>
#include <memory>
#include <algorithm>
#include <iomanip>
#include <cassert>

// ─────────────────────────────────────────────
//  MATH PRIMITIVES
// ─────────────────────────────────────────────

const double PI = 3.14159265358979323846;

struct Vec3 {
    double x, y, z;
    Vec3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(double s)      const { return {x*s,   y*s,   z*s  }; }
    Vec3 lerp(const Vec3& o, double t) const { return *this + (o - *this) * t; }
    double length() const { return std::sqrt(x*x + y*y + z*z); }

    Vec3 normalized() const {
        double len = length();
        return (len > 1e-9) ? Vec3(x/len, y/len, z/len) : Vec3();
    }

    void print(const std::string& label = "") const {
        if (!label.empty()) std::cout << label << ": ";
        std::cout << std::fixed << std::setprecision(3)
                  << "(" << x << ", " << y << ", " << z << ")";
    }
};

// ─────────────────────────────────────────────
//  QUATERNION  (rotation without gimbal lock)
// ─────────────────────────────────────────────

struct Quat {
    double w, x, y, z;
    Quat(double w=1,double x=0,double y=0,double z=0): w(w),x(x),y(y),z(z){}

    // Build from axis-angle (angle in radians)
    static Quat fromAxisAngle(Vec3 axis, double angle) {
        axis = axis.normalized();
        double s = std::sin(angle * 0.5);
        return { std::cos(angle * 0.5), axis.x*s, axis.y*s, axis.z*s };
    }

    Quat operator*(const Quat& o) const {
        return {
            w*o.w - x*o.x - y*o.y - z*o.z,
            w*o.x + x*o.w + y*o.z - z*o.y,
            w*o.y - x*o.z + y*o.w + z*o.x,
            w*o.z + x*o.y - y*o.x + z*o.w
        };
    }

    Quat normalized() const {
        double n = std::sqrt(w*w+x*x+y*y+z*z);
        return (n>1e-9) ? Quat(w/n,x/n,y/n,z/n) : Quat();
    }

    // Rotate a point
    Vec3 rotate(const Vec3& v) const {
        Quat p(0, v.x, v.y, v.z);
        Quat conj(w,-x,-y,-z);
        Quat res = (*this * p) * conj;
        return {res.x, res.y, res.z};
    }

    // Spherical linear interpolation
    static Quat slerp(Quat a, Quat b, double t) {
        double dot = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
        if (dot < 0) { b = Quat(-b.w,-b.x,-b.y,-b.z); dot = -dot; }
        if (dot > 0.9995) {
            // Linear fallback
            Quat r(a.w + t*(b.w-a.w), a.x + t*(b.x-a.x),
                   a.y + t*(b.y-a.y), a.z + t*(b.z-a.z));
            return r.normalized();
        }
        double theta0 = std::acos(dot);
        double theta  = theta0 * t;
        double s0 = std::cos(theta) - dot * std::sin(theta) / std::sin(theta0);
        double s1 = std::sin(theta) / std::sin(theta0);
        return Quat(s0*a.w + s1*b.w, s0*a.x + s1*b.x,
                    s0*a.y + s1*b.y, s0*a.z + s1*b.z).normalized();
    }

    void print() const {
        std::cout << std::fixed << std::setprecision(3)
                  << "[w=" << w << " x=" << x << " y=" << y << " z=" << z << "]";
    }
};

// ─────────────────────────────────────────────
//  BONE  (single skeletal segment)
// ─────────────────────────────────────────────

struct Bone {
    std::string  name;
    double       length;          // bone length in arbitrary units
    Vec3         localOffset;     // rest-pose offset from parent origin
    Quat         localRotation;   // current local rotation
    Vec3         worldPosition;   // computed by FK
    Vec3         worldTip;        // tip of bone in world space

    // Rotation limits (Euler in degrees for readability)
    double minAngle, maxAngle;

    std::vector<std::shared_ptr<Bone>> children;

    Bone(const std::string& name, double len,
         Vec3 offset = {0,0,0},
         double minA = -180, double maxA = 180)
        : name(name), length(len), localOffset(offset),
          localRotation(), worldPosition(), worldTip(),
          minAngle(minA), maxAngle(maxA) {}

    void addChild(std::shared_ptr<Bone> child) {
        children.push_back(child);
    }

    // Apply a rotation (axis-angle, degrees) — clamped to limits
    void rotate(Vec3 axis, double degrees) {
        double rad = degrees * PI / 180.0;
        // Clamp
        degrees = std::clamp(degrees, minAngle, maxAngle);
        rad = degrees * PI / 180.0;
        Quat delta = Quat::fromAxisAngle(axis, rad);
        localRotation = (localRotation * delta).normalized();
    }

    void resetRotation() { localRotation = Quat(); }
};

// ─────────────────────────────────────────────
//  FORWARD KINEMATICS  (recursive)
// ─────────────────────────────────────────────

void forwardKinematics(std::shared_ptr<Bone> bone,
                       Vec3 parentPos, Quat parentRot) {
    // World rotation = parent rotation * local rotation
    Quat worldRot = (parentRot * bone->localRotation).normalized();

    // World position = parent position + rotate localOffset by parentRot
    bone->worldPosition = parentPos + parentRot.rotate(bone->localOffset);

    // Tip = worldPosition + worldRot applied to (0, length, 0)
    bone->worldTip = bone->worldPosition + worldRot.rotate({0, bone->length, 0});

    for (auto& child : bone->children)
        forwardKinematics(child, bone->worldTip, worldRot);
}

// ─────────────────────────────────────────────
//  KEYFRAME  (pose snapshot)
// ─────────────────────────────────────────────

struct BonePose {
    std::string boneName;
    Quat        rotation;
};

struct Keyframe {
    double              time;     // in seconds
    std::vector<BonePose> poses;
};

// ─────────────────────────────────────────────
//  ANIMATION CLIP
// ─────────────────────────────────────────────

class AnimationClip {
public:
    std::string          name;
    double               duration;
    std::vector<Keyframe> keyframes;

    AnimationClip(const std::string& n, double dur)
        : name(n), duration(dur) {}

    void addKeyframe(Keyframe kf) {
        keyframes.push_back(kf);
        std::sort(keyframes.begin(), keyframes.end(),
            [](const Keyframe& a, const Keyframe& b){ return a.time < b.time; });
    }

    // Returns interpolated poses at a given time
    std::vector<BonePose> sample(double t) const {
        t = std::fmod(t, duration);   // loop

        if (keyframes.empty()) return {};
        if (keyframes.size() == 1 || t <= keyframes.front().time)
            return keyframes.front().poses;
        if (t >= keyframes.back().time)
            return keyframes.back().poses;

        // Find surrounding keyframes
        for (size_t i = 0; i + 1 < keyframes.size(); ++i) {
            const Keyframe& a = keyframes[i];
            const Keyframe& b = keyframes[i+1];
            if (t >= a.time && t <= b.time) {
                double alpha = (t - a.time) / (b.time - a.time);
                // Slerp each bone rotation
                std::vector<BonePose> blended;
                for (size_t j = 0; j < a.poses.size() && j < b.poses.size(); ++j) {
                    BonePose bp;
                    bp.boneName = a.poses[j].boneName;
                    bp.rotation = Quat::slerp(a.poses[j].rotation,
                                              b.poses[j].rotation, alpha);
                    blended.push_back(bp);
                }
                return blended;
            }
        }
        return keyframes.back().poses;
    }
};

// ─────────────────────────────────────────────
//  SKELETON BUILDER  (full arm + hand)
// ─────────────────────────────────────────────

struct ArmSkeleton {
    // Upper arm
    std::shared_ptr<Bone> shoulder;
    std::shared_ptr<Bone> upperArm;
    std::shared_ptr<Bone> elbow;
    std::shared_ptr<Bone> foreArm;
    std::shared_ptr<Bone> wrist;
    std::shared_ptr<Bone> palm;

    // 5 fingers × 3 joints each
    struct Finger {
        std::string name;
        std::shared_ptr<Bone> metacarpal;   // knuckle base
        std::shared_ptr<Bone> proximal;     // first phalanx
        std::shared_ptr<Bone> middle;       // second phalanx
        std::shared_ptr<Bone> distal;       // fingertip phalanx
    };
    std::vector<Finger> fingers;   // thumb, index, middle, ring, pinky

    // Map for quick lookup by name
    std::map<std::string, std::shared_ptr<Bone>> boneMap;

    void registerBone(std::shared_ptr<Bone> b) {
        boneMap[b->name] = b;
    }
};

ArmSkeleton buildArmSkeleton(bool isRight = true) {
    ArmSkeleton arm;
    std::string side = isRight ? "R" : "L";
    double mirror = isRight ? 1.0 : -1.0;

    // ── Shoulder joint ──
    arm.shoulder = std::make_shared<Bone>(side+"_Shoulder", 0.0,
                                          Vec3(mirror*15, 140, 0), -90, 90);

    // ── Upper arm ──
    arm.upperArm = std::make_shared<Bone>(side+"_UpperArm", 28,
                                           Vec3(0,0,0), -180, 180);

    // ── Elbow joint ──
    arm.elbow = std::make_shared<Bone>(side+"_Elbow", 0.0,
                                        Vec3(0,0,0), 0, 145);

    // ── Forearm ──
    arm.foreArm = std::make_shared<Bone>(side+"_ForeArm", 24,
                                          Vec3(0,0,0), -90, 90);

    // ── Wrist ──
    arm.wrist = std::make_shared<Bone>(side+"_Wrist", 3,
                                        Vec3(0,0,0), -70, 70);

    // ── Palm ──
    arm.palm = std::make_shared<Bone>(side+"_Palm", 8,
                                       Vec3(0,0,0), -30, 30);

    // ── Fingers ──
    // (name, metacarpal_len, prox_len, mid_len, dist_len, x_offset)
    struct FingerDef {
        std::string name;
        double mc, pr, mi, di;
        double xOff;
    };
    std::vector<FingerDef> fdefs = {
        {"Thumb",  3.5, 3.0, 2.5, 0, mirror* 4.0},
        {"Index",  2.0, 4.0, 3.0, 2.0, mirror* 2.0},
        {"Middle", 2.0, 4.5, 3.5, 2.5, mirror* 0.5},
        {"Ring",   2.0, 4.0, 3.0, 2.0, mirror*-1.0},
        {"Pinky",  1.5, 3.0, 2.0, 1.5, mirror*-2.5}
    };

    for (auto& fd : fdefs) {
        ArmSkeleton::Finger f;
        f.name = fd.name;
        std::string fp = side + "_" + fd.name;

        f.metacarpal = std::make_shared<Bone>(fp+"_Metacarpal", fd.mc,
                                               Vec3(fd.xOff, 0, 0), -20, 20);
        f.proximal   = std::make_shared<Bone>(fp+"_Proximal",   fd.pr,
                                               Vec3(0,0,0), -90, 10);
        f.middle     = std::make_shared<Bone>(fp+"_Middle",     fd.mi,
                                               Vec3(0,0,0), -90, 10);
        f.distal     = std::make_shared<Bone>(fp+"_Distal",     fd.di,
                                               Vec3(0,0,0), -80, 10);

        // Link finger chain
        f.metacarpal->addChild(f.proximal);
        f.proximal->addChild(f.middle);
        if (fd.di > 0) f.middle->addChild(f.distal);

        arm.palm->addChild(f.metacarpal);

        arm.registerBone(f.metacarpal);
        arm.registerBone(f.proximal);
        arm.registerBone(f.middle);
        if (fd.di > 0) arm.registerBone(f.distal);

        arm.fingers.push_back(f);
    }

    // ── Link arm chain ──
    arm.shoulder->addChild(arm.upperArm);
    arm.upperArm->addChild(arm.elbow);
    arm.elbow->addChild(arm.foreArm);
    arm.foreArm->addChild(arm.wrist);
    arm.wrist->addChild(arm.palm);

    arm.registerBone(arm.shoulder);
    arm.registerBone(arm.upperArm);
    arm.registerBone(arm.elbow);
    arm.registerBone(arm.foreArm);
    arm.registerBone(arm.wrist);
    arm.registerBone(arm.palm);

    return arm;
}

// ─────────────────────────────────────────────
//  APPLY POSES TO SKELETON
// ─────────────────────────────────────────────

void applyPoses(ArmSkeleton& arm, const std::vector<BonePose>& poses) {
    for (auto& bp : poses) {
        auto it = arm.boneMap.find(bp.boneName);
        if (it != arm.boneMap.end())
            it->second->localRotation = bp.rotation;
    }
}

// ─────────────────────────────────────────────
//  PRINT SKELETON STATE
// ─────────────────────────────────────────────

void printBoneTree(std::shared_ptr<Bone> bone, int depth = 0) {
    std::string indent(depth * 3, ' ');
    std::cout << indent << "├─ [" << bone->name << "]"
              << "  len=" << std::fixed << std::setprecision(1) << bone->length;
    std::cout << "  pos=";
    bone->worldPosition.print();
    std::cout << "  tip=";
    bone->worldTip.print();
    std::cout << "\n";
    for (auto& c : bone->children)
        printBoneTree(c, depth + 1);
}

// ─────────────────────────────────────────────
//  PRESET POSE BUILDERS
// ─────────────────────────────────────────────

// Helper: axis-angle → BonePose
BonePose makePose(const std::string& name, Vec3 axis, double degrees) {
    return { name, Quat::fromAxisAngle(axis, degrees * PI / 180.0) };
}

// T-Pose (rest)
std::vector<BonePose> poseTPose(const std::string& side) {
    // All identity quaternions
    std::vector<BonePose> poses;
    for (auto& n : {side+"_Shoulder", side+"_UpperArm", side+"_Elbow",
                    side+"_ForeArm",  side+"_Wrist",    side+"_Palm"})
        poses.push_back({n, Quat()});
    for (auto& f : {"Thumb","Index","Middle","Ring","Pinky"})
        for (auto& j : {"_Metacarpal","_Proximal","_Middle","_Distal"})
            poses.push_back({side+"_"+f+j, Quat()});
    return poses;
}

// Wave gesture
std::vector<BonePose> poseWave(const std::string& side) {
    auto p = poseTPose(side);
    // Raise arm sideways
    p.push_back(makePose(side+"_Shoulder",  {0,0,1},  80));
    p.push_back(makePose(side+"_UpperArm",  {1,0,0}, -20));
    // Bend elbow
    p.push_back(makePose(side+"_Elbow",     {1,0,0},  90));
    // Slight wrist tilt
    p.push_back(makePose(side+"_Wrist",     {0,0,1},  20));
    // Fingers spread open
    for (auto& f : {"Index","Middle","Ring","Pinky"}) {
        p.push_back(makePose(side+"_"+f+"_Proximal", {1,0,0}, -10));
        p.push_back(makePose(side+"_"+f+"_Middle",   {1,0,0}, -5));
    }
    return p;
}

// Fist / punch
std::vector<BonePose> poseFist(const std::string& side) {
    auto p = poseTPose(side);
    p.push_back(makePose(side+"_Shoulder", {0,0,1},   30));
    p.push_back(makePose(side+"_Elbow",    {1,0,0},   70));
    p.push_back(makePose(side+"_Wrist",    {0,0,1},  -10));
    for (auto& f : {"Index","Middle","Ring","Pinky"}) {
        p.push_back(makePose(side+"_"+f+"_Proximal", {1,0,0}, -80));
        p.push_back(makePose(side+"_"+f+"_Middle",   {1,0,0}, -70));
        p.push_back(makePose(side+"_"+f+"_Distal",   {1,0,0}, -50));
    }
    // Thumb wrap
    p.push_back(makePose(side+"_Thumb_Proximal", {0,1,0}, -40));
    p.push_back(makePose(side+"_Thumb_Middle",   {1,0,0}, -30));
    return p;
}

// Point / index finger extended
std::vector<BonePose> posePoint(const std::string& side) {
    auto p = poseFist(side);   // start from fist
    // Extend index finger
    p.push_back(makePose(side+"_Index_Proximal", {1,0,0},  0));
    p.push_back(makePose(side+"_Index_Middle",   {1,0,0},  0));
    p.push_back(makePose(side+"_Index_Distal",   {1,0,0},  0));
    return p;
}

// Thumbs up
std::vector<BonePose> poseThumbsUp(const std::string& side) {
    auto p = poseFist(side);
    p.push_back(makePose(side+"_Shoulder", {0,0,1},  15));
    p.push_back(makePose(side+"_Elbow",    {1,0,0},  40));
    // Extend thumb
    p.push_back(makePose(side+"_Thumb_Metacarpal", {0,0,1},  20));
    p.push_back(makePose(side+"_Thumb_Proximal",   {1,0,0}, -70));
    p.push_back(makePose(side+"_Thumb_Middle",     {1,0,0},  0));
    return p;
}

// Open hand / stop
std::vector<BonePose> poseOpenHand(const std::string& side) {
    auto p = poseTPose(side);
    p.push_back(makePose(side+"_Shoulder", {0,0,1},  50));
    p.push_back(makePose(side+"_Elbow",    {1,0,0},  60));
    p.push_back(makePose(side+"_Wrist",    {0,0,1},   5));
    for (auto& f : {"Thumb","Index","Middle","Ring","Pinky"}) {
        p.push_back(makePose(side+"_"+f+"_Proximal", {1,0,0},  5));
        p.push_back(makePose(side+"_"+f+"_Middle",   {1,0,0},  3));
        p.push_back(makePose(side+"_"+f+"_Distal",   {1,0,0},  2));
    }
    return p;
}

// ─────────────────────────────────────────────
//  ANIMATION CLIP FACTORY
// ─────────────────────────────────────────────

AnimationClip buildWaveAnimation(const std::string& side) {
    AnimationClip clip("Wave_"+side, 2.0);

    Keyframe kf0; kf0.time = 0.0;
    for (auto& bp : poseOpenHand(side)) kf0.poses.push_back(bp);
    clip.addKeyframe(kf0);

    // Mid-wave: wrist rotated
    Keyframe kf1; kf1.time = 0.5;
    for (auto& bp : poseOpenHand(side)) kf1.poses.push_back(bp);
    // Override wrist
    kf1.poses.push_back(makePose(side+"_Wrist", {0,0,1}, -25));
    clip.addKeyframe(kf1);

    // Back
    Keyframe kf2; kf2.time = 1.0;
    for (auto& bp : poseOpenHand(side)) kf2.poses.push_back(bp);
    kf2.poses.push_back(makePose(side+"_Wrist", {0,0,1},  25));
    clip.addKeyframe(kf2);

    Keyframe kf3; kf3.time = 2.0;
    for (auto& bp : poseOpenHand(side)) kf3.poses.push_back(bp);
    clip.addKeyframe(kf3);

    return clip;
}

AnimationClip buildPunchAnimation(const std::string& side) {
    AnimationClip clip("Punch_"+side, 1.0);

    Keyframe wind; wind.time = 0.0;
    for (auto& bp : poseTPose(side)) wind.poses.push_back(bp);
    wind.poses.push_back(makePose(side+"_Shoulder", {0,0,1},  -20));
    wind.poses.push_back(makePose(side+"_Elbow",    {1,0,0},  110));
    for (auto& bp : poseFist(side)) wind.poses.push_back(bp);
    clip.addKeyframe(wind);

    Keyframe extend; extend.time = 0.4;
    for (auto& bp : poseFist(side)) extend.poses.push_back(bp);
    extend.poses.push_back(makePose(side+"_Shoulder", {0,0,1},  10));
    extend.poses.push_back(makePose(side+"_Elbow",    {1,0,0},  15));
    clip.addKeyframe(extend);

    Keyframe recoil; recoil.time = 1.0;
    for (auto& bp : poseTPose(side)) recoil.poses.push_back(bp);
    clip.addKeyframe(recoil);

    return clip;
}

// ─────────────────────────────────────────────
//  ANIMATOR  (drives clips on a skeleton)
// ─────────────────────────────────────────────

class Animator {
public:
    ArmSkeleton&   skeleton;
    AnimationClip* currentClip = nullptr;
    double         playhead    = 0.0;
    double         speed       = 1.0;
    bool           playing     = false;

    explicit Animator(ArmSkeleton& skel) : skeleton(skel) {}

    void play(AnimationClip& clip, double spd = 1.0) {
        currentClip = &clip;
        playhead    = 0.0;
        speed       = spd;
        playing     = true;
    }

    void update(double dt) {
        if (!playing || !currentClip) return;
        playhead += dt * speed;
        auto poses = currentClip->sample(playhead);
        applyPoses(skeleton, poses);
        forwardKinematics(skeleton.shoulder, {0,0,0}, Quat());
    }

    void stop() { playing = false; }
};

// ─────────────────────────────────────────────
//  DEMO: SIMULATE ANIMATION FRAMES
// ─────────────────────────────────────────────

void simulateClip(ArmSkeleton& arm, AnimationClip& clip,
                  int frameCount = 10, double fps = 30.0) {
    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout <<   "║  CLIP: " << std::left << std::setw(34) << clip.name << "║\n";
    std::cout <<   "╚══════════════════════════════════════════╝\n";

    Animator animator(arm);
    animator.play(clip);

    double dt = 1.0 / fps;
    for (int frame = 0; frame < frameCount; ++frame) {
        animator.update(dt);
        double t = frame * dt;
        std::cout << "\n── Frame " << std::setw(3) << frame
                  << "  (t=" << std::fixed << std::setprecision(3) << t << "s) ──\n";
        printBoneTree(arm.shoulder);
    }
}

// ─────────────────────────────────────────────
//  MANUAL POSE DEMO
// ─────────────────────────────────────────────

void demoPose(ArmSkeleton& arm, const std::string& poseName,
              const std::vector<BonePose>& poses) {
    std::cout << "\n┌─────────────────────────────────────────┐\n";
    std::cout << "│  POSE: " << std::left << std::setw(33) << poseName << "│\n";
    std::cout << "└─────────────────────────────────────────┘\n";
    applyPoses(arm, poses);
    forwardKinematics(arm.shoulder, {0,0,0}, Quat());
    printBoneTree(arm.shoulder);
}

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────

int main() {
    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║   CHARACTER ARM & HAND ANIMATION SYSTEM      ║\n";
    std::cout << "║   Forward Kinematics + Keyframe Animation     ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n";

    // ── Build right-arm skeleton ──
    ArmSkeleton rightArm = buildArmSkeleton(true);

    // ── Static pose demos ──
    demoPose(rightArm, "T-Pose",    poseTPose("R"));
    demoPose(rightArm, "Fist",      poseFist("R"));
    demoPose(rightArm, "Point",     posePoint("R"));
    demoPose(rightArm, "ThumbsUp",  poseThumbsUp("R"));
    demoPose(rightArm, "OpenHand",  poseOpenHand("R"));
    demoPose(rightArm, "Wave",      poseWave("R"));

    // ── Animated clip demos ──
    AnimationClip waveClip  = buildWaveAnimation("R");
    AnimationClip punchClip = buildPunchAnimation("R");

    simulateClip(rightArm, waveClip,  8, 24.0);
    simulateClip(rightArm, punchClip, 8, 24.0);

    // ── Build left-arm skeleton ──
    std::cout << "\n\n═══ LEFT ARM ═══\n";
    ArmSkeleton leftArm = buildArmSkeleton(false);
    demoPose(leftArm, "Fist (Left)", poseFist("L"));

    std::cout << "\n✓ Animation simulation complete.\n";
    return 0;
}
