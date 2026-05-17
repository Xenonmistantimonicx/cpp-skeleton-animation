/*
 * ============================================================
 *  CHARACTER HEAD, HAIR & FACIAL ANIMATION SYSTEM
 *  Covers: Head, Neck, Jaw, Eyes, Eyelids, Eyebrows,
 *          Cheeks, Nose, Lips, Tongue, Hair Strands (physics)
 *  Uses: Forward Kinematics + Blend Shapes + Hair Simulation
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
#include <functional>
#include <cassert>

// ─────────────────────────────────────────────
//  MATH PRIMITIVES
// ─────────────────────────────────────────────

const double PI = 3.14159265358979323846;

struct Vec3 {
    double x, y, z;
    Vec3(double x=0,double y=0,double z=0):x(x),y(y),z(z){}
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(double s)      const { return {x*s,   y*s,   z*s  }; }
    Vec3 lerp(const Vec3& o, double t) const { return *this + (o-*this)*t; }
    double length() const { return std::sqrt(x*x+y*y+z*z); }
    Vec3 normalized() const {
        double l=length(); return (l>1e-9)?Vec3(x/l,y/l,z/l):Vec3();
    }
    void print(const std::string& lbl="") const {
        if(!lbl.empty()) std::cout<<lbl<<": ";
        std::cout<<std::fixed<<std::setprecision(3)
                 <<"("<<x<<", "<<y<<", "<<z<<")";
    }
};

// ─────────────────────────────────────────────
//  QUATERNION
// ─────────────────────────────────────────────

struct Quat {
    double w,x,y,z;
    Quat(double w=1,double x=0,double y=0,double z=0):w(w),x(x),y(y),z(z){}

    static Quat fromAxisAngle(Vec3 axis, double angle){
        axis=axis.normalized();
        double s=std::sin(angle*0.5);
        return {std::cos(angle*0.5), axis.x*s, axis.y*s, axis.z*s};
    }
    Quat operator*(const Quat& o) const {
        return { w*o.w-x*o.x-y*o.y-z*o.z,
                 w*o.x+x*o.w+y*o.z-z*o.y,
                 w*o.y-x*o.z+y*o.w+z*o.x,
                 w*o.z+x*o.y-y*o.x+z*o.w };
    }
    Quat normalized() const {
        double n=std::sqrt(w*w+x*x+y*y+z*z);
        return (n>1e-9)?Quat(w/n,x/n,y/n,z/n):Quat();
    }
    Vec3 rotate(const Vec3& v) const {
        Quat p(0,v.x,v.y,v.z), conj(w,-x,-y,-z);
        Quat r=(*this*p)*conj;
        return {r.x,r.y,r.z};
    }
    static Quat slerp(Quat a, Quat b, double t){
        double dot=a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z;
        if(dot<0){b=Quat(-b.w,-b.x,-b.y,-b.z);dot=-dot;}
        if(dot>0.9995){
            Quat r(a.w+t*(b.w-a.w),a.x+t*(b.x-a.x),
                   a.y+t*(b.y-a.y),a.z+t*(b.z-a.z));
            return r.normalized();
        }
        double th0=std::acos(dot), th=th0*t;
        double s0=std::cos(th)-dot*std::sin(th)/std::sin(th0);
        double s1=std::sin(th)/std::sin(th0);
        return Quat(s0*a.w+s1*b.w,s0*a.x+s1*b.x,
                    s0*a.y+s1*b.y,s0*a.z+s1*b.z).normalized();
    }
};

// ─────────────────────────────────────────────
//  BONE
// ─────────────────────────────────────────────

struct Bone {
    std::string  name;
    double       length;
    Vec3         localOffset;
    Quat         localRotation;
    Vec3         worldPosition;
    Vec3         worldTip;
    double       minAngle, maxAngle;
    std::vector<std::shared_ptr<Bone>> children;

    Bone(const std::string& n, double len,
         Vec3 offset={}, double mn=-180, double mx=180)
        : name(n), length(len), localOffset(offset),
          localRotation(), worldPosition(), worldTip(),
          minAngle(mn), maxAngle(mx) {}

    void addChild(std::shared_ptr<Bone> c){ children.push_back(c); }
    void resetRotation(){ localRotation=Quat(); }
};

// ─────────────────────────────────────────────
//  FORWARD KINEMATICS
// ─────────────────────────────────────────────

void forwardKinematics(std::shared_ptr<Bone> bone,
                       Vec3 parentPos, Quat parentRot){
    Quat worldRot=(parentRot*bone->localRotation).normalized();
    bone->worldPosition=parentPos+parentRot.rotate(bone->localOffset);
    bone->worldTip=bone->worldPosition+worldRot.rotate({0,bone->length,0});
    for(auto& c:bone->children)
        forwardKinematics(c, bone->worldTip, worldRot);
}

// ─────────────────────────────────────────────
//  BLEND SHAPE  (morph target for facial expressions)
// ─────────────────────────────────────────────

struct BlendShape {
    std::string name;
    double      weight;       // 0.0 = no influence, 1.0 = full
    double      minW, maxW;

    BlendShape(const std::string& n, double mn=0.0, double mx=1.0)
        : name(n), weight(0.0), minW(mn), maxW(mx) {}

    void set(double w){ weight=std::clamp(w, minW, maxW); }
    double get() const { return weight; }

    // Blend two weights
    static double blend(double a, double b, double t){
        return a + (b-a)*t;
    }
};

// ─────────────────────────────────────────────
//  HAIR STRAND  (simple spring-chain physics)
// ─────────────────────────────────────────────

struct HairParticle {
    Vec3   position;
    Vec3   prevPosition;
    Vec3   velocity;
    double mass;
    bool   pinned;   // root particle is pinned to scalp

    HairParticle(Vec3 pos, double m=1.0, bool pin=false)
        : position(pos), prevPosition(pos), velocity(), mass(m), pinned(pin) {}
};

struct HairStrand {
    std::string               name;
    std::vector<HairParticle> particles;
    double                    segmentLength;
    double                    stiffness;     // 0=floppy, 1=rigid
    double                    damping;

    HairStrand(const std::string& n, Vec3 root,
               int segments=6, double segLen=1.2,
               double stiff=0.3, double damp=0.85)
        : name(n), segmentLength(segLen), stiffness(stiff), damping(damp)
    {
        // Build chain downward from root
        for(int i=0;i<=segments;++i){
            Vec3 p = root + Vec3(0, -segLen*i, 0);
            particles.emplace_back(p, 1.0, i==0);
        }
    }

    // Simulate one step (Verlet integration)
    void simulate(double dt, Vec3 gravity={0,-9.8,0}, Vec3 wind={0,0,0}){
        Vec3 extForce = gravity + wind;

        for(auto& p : particles){
            if(p.pinned) continue;
            Vec3 vel     = (p.position - p.prevPosition) * damping;
            Vec3 newPos  = p.position + vel + extForce*(dt*dt);
            p.prevPosition = p.position;
            p.position     = newPos;
        }

        // Constraint: maintain segment lengths (iterative)
        for(int iter=0;iter<5;++iter){
            for(size_t i=0;i+1<particles.size();++i){
                auto& a = particles[i];
                auto& b = particles[i+1];
                Vec3 delta = b.position - a.position;
                double dist = delta.length();
                if(dist < 1e-9) continue;
                double error = (dist - segmentLength) / dist;
                Vec3 corr = delta * error * 0.5;
                if(!a.pinned) a.position = a.position + corr;
                if(!b.pinned) b.position = b.position - corr;
            }
        }

        // Stiffness: pull toward rest pose
        for(size_t i=1;i<particles.size();++i){
            Vec3 rest = particles[i-1].position + Vec3(0,-segmentLength,0);
            particles[i].position =
                particles[i].position.lerp(rest, stiffness*dt);
        }
    }

    // Move root (attached to head bone)
    void setRoot(Vec3 newRoot){
        Vec3 delta = newRoot - particles[0].position;
        for(auto& p : particles) p.position = p.position + delta;
        particles[0].position   = newRoot;
        particles[0].prevPosition = newRoot;
    }

    void print() const {
        std::cout << "  Strand [" << name << "] particles:\n";
        for(size_t i=0;i<particles.size();++i){
            std::cout << "    [" << i << "] ";
            particles[i].position.print();
            std::cout << (particles[i].pinned?" (pinned)":"") << "\n";
        }
    }
};

// ─────────────────────────────────────────────
//  EYE CONTROLLER
// ─────────────────────────────────────────────

struct EyeController {
    std::string name;
    Vec3        position;        // world position of eye center
    Vec3        lookTarget;      // where the eye is looking
    double      pupilDilation;   // 0=constricted, 1=dilated
    double      irisColor[3];    // RGB (0-1)

    // Blend shapes for this eye
    BlendShape  blinkUpper;
    BlendShape  blinkLower;
    BlendShape  squint;
    BlendShape  wideOpen;

    EyeController(const std::string& n, Vec3 pos)
        : name(n), position(pos), lookTarget(pos+Vec3(0,0,10)),
          pupilDilation(0.5),
          blinkUpper(n+"_BlinkUpper"),
          blinkLower(n+"_BlinkLower"),
          squint(n+"_Squint"),
          wideOpen(n+"_WideOpen")
    {
        irisColor[0]=0.2; irisColor[1]=0.5; irisColor[2]=0.9;
    }

    Vec3 getLookDirection() const {
        return (lookTarget - position).normalized();
    }

    void blink(double amount=1.0){
        blinkUpper.set(amount);
        blinkLower.set(amount*0.6);
    }
    void openEye(){ blinkUpper.set(0); blinkLower.set(0); }

    void print() const {
        std::cout << "  Eye [" << name << "]"
                  << "  blink=" << std::fixed<<std::setprecision(2)
                  << blinkUpper.get()
                  << "  squint=" << squint.get()
                  << "  pupil=" << pupilDilation << "\n";
        std::cout << "    Look dir: ";
        getLookDirection().print(); std::cout<<"\n";
    }
};

// ─────────────────────────────────────────────
//  FACIAL EXPRESSION SYSTEM  (blend shape set)
// ─────────────────────────────────────────────

struct FacialRig {
    // ── Brow ──
    BlendShape browRaiseLeft   {"BrowRaiseL",  0,1};
    BlendShape browRaiseRight  {"BrowRaiseR",  0,1};
    BlendShape browFurrow      {"BrowFurrow",  0,1};
    BlendShape browLowerLeft   {"BrowLowerL",  0,1};
    BlendShape browLowerRight  {"BrowLowerR",  0,1};

    // ── Eyes ──
    BlendShape eyeSquintLeft   {"EyeSquintL",  0,1};
    BlendShape eyeSquintRight  {"EyeSquintR",  0,1};
    BlendShape eyeWideLeft     {"EyeWideL",    0,1};
    BlendShape eyeWideRight    {"EyeWideR",    0,1};

    // ── Nose ──
    BlendShape noseSneer       {"NoseSneer",   0,1};
    BlendShape nostrilFlare    {"NostrilFlare",0,1};
    BlendShape noseWrinkle     {"NoseWrinkle", 0,1};

    // ── Cheeks ──
    BlendShape cheekPuffLeft   {"CheekPuffL",  0,1};
    BlendShape cheekPuffRight  {"CheekPuffR",  0,1};
    BlendShape cheekRaiseLeft  {"CheekRaiseL", 0,1};
    BlendShape cheekRaiseRight {"CheekRaiseR", 0,1};

    // ── Lips / Mouth ──
    BlendShape mouthSmileLeft  {"SmileL",      0,1};
    BlendShape mouthSmileRight {"SmileR",      0,1};
    BlendShape mouthFrownLeft  {"FrownL",      0,1};
    BlendShape mouthFrownRight {"FrownR",      0,1};
    BlendShape mouthOpen       {"MouthOpen",   0,1};
    BlendShape mouthPucker     {"MouthPucker", 0,1};
    BlendShape mouthStretch    {"MouthStretch",0,1};
    BlendShape lipUpperUp      {"LipUpperUp",  0,1};
    BlendShape lipLowerDown    {"LipLowerDown",0,1};
    BlendShape lipCornerPull   {"LipCornerPull",0,1};
    BlendShape lipPress        {"LipPress",    0,1};

    // ── Jaw ──
    BlendShape jawOpen         {"JawOpen",     0,1};
    BlendShape jawLeft         {"JawLeft",    -1,1};
    BlendShape jawRight        {"JawRight",   -1,1};
    BlendShape jawForward      {"JawForward",  0,1};

    // ── Tongue ──
    BlendShape tongueOut       {"TongueOut",   0,1};
    BlendShape tongueUp        {"TongueUp",   -1,1};
    BlendShape tongueSide      {"TongueSide", -1,1};

    void resetAll(){
        for(auto* bs : allShapes()) bs->set(0);
    }

    std::vector<BlendShape*> allShapes(){
        return { &browRaiseLeft,&browRaiseRight,&browFurrow,
                 &browLowerLeft,&browLowerRight,
                 &eyeSquintLeft,&eyeSquintRight,
                 &eyeWideLeft,&eyeWideRight,
                 &noseSneer,&nostrilFlare,&noseWrinkle,
                 &cheekPuffLeft,&cheekPuffRight,
                 &cheekRaiseLeft,&cheekRaiseRight,
                 &mouthSmileLeft,&mouthSmileRight,
                 &mouthFrownLeft,&mouthFrownRight,
                 &mouthOpen,&mouthPucker,&mouthStretch,
                 &lipUpperUp,&lipLowerDown,&lipCornerPull,&lipPress,
                 &jawOpen,&jawLeft,&jawRight,&jawForward,
                 &tongueOut,&tongueUp,&tongueSide };
    }

    void print() const {
        std::cout << "  ── Facial Blend Shapes ──\n";
        auto printBS = [](const BlendShape& b){
            if(b.get()>0.01)
                std::cout << "    " << std::left<<std::setw(20)<<b.name
                          << " = " << std::fixed<<std::setprecision(3)
                          << b.get() << "\n";
        };
        printBS(browRaiseLeft);  printBS(browRaiseRight);
        printBS(browFurrow);     printBS(browLowerLeft);
        printBS(eyeSquintLeft);  printBS(eyeSquintRight);
        printBS(eyeWideLeft);    printBS(eyeWideRight);
        printBS(noseSneer);      printBS(nostrilFlare);
        printBS(cheekPuffLeft);  printBS(cheekPuffRight);
        printBS(cheekRaiseLeft); printBS(cheekRaiseRight);
        printBS(mouthSmileLeft); printBS(mouthSmileRight);
        printBS(mouthFrownLeft); printBS(mouthFrownRight);
        printBS(mouthOpen);      printBS(mouthPucker);
        printBS(jawOpen);        printBS(jawForward);
        printBS(tongueOut);      printBS(tongueSide);
    }
};

// ─────────────────────────────────────────────
//  HEAD SKELETON  (bones)
// ─────────────────────────────────────────────

struct HeadSkeleton {
    // ── Neck / Head ──
    std::shared_ptr<Bone> cervical1;    // atlas (top vertebra)
    std::shared_ptr<Bone> cervical2;    // axis
    std::shared_ptr<Bone> neck;
    std::shared_ptr<Bone> head;
    std::shared_ptr<Bone> skull;

    // ── Jaw ──
    std::shared_ptr<Bone> jaw;
    std::shared_ptr<Bone> chinTip;

    // ── Eye bones (for look-at) ──
    std::shared_ptr<Bone> eyeLeft;
    std::shared_ptr<Bone> eyeRight;

    // ── Eyebrow bones ──
    std::shared_ptr<Bone> browLeftInner, browLeftMid, browLeftOuter;
    std::shared_ptr<Bone> browRightInner,browRightMid,browRightOuter;

    // ── Nose ──
    std::shared_ptr<Bone> noseRoot;
    std::shared_ptr<Bone> noseTip;
    std::shared_ptr<Bone> nostrilLeft, nostrilRight;

    // ── Lip corners ──
    std::shared_ptr<Bone> lipCornerLeft, lipCornerRight;
    std::shared_ptr<Bone> lipUpperMid,   lipLowerMid;

    // ── Cheek ──
    std::shared_ptr<Bone> cheekLeft, cheekRight;

    // ── Tongue ──
    std::shared_ptr<Bone> tongueRoot, tongueMid, tongueTip;

    // ── Ear ──
    std::shared_ptr<Bone> earLeft, earRight;

    // Facial rig (blend shapes)
    FacialRig facial;

    // Eyes
    EyeController leftEye  {"LeftEye",  Vec3(-3, 160, 9)};
    EyeController rightEye {"RightEye", Vec3( 3, 160, 9)};

    // Hair strands
    std::vector<HairStrand> hairStrands;

    // Bone map
    std::map<std::string,std::shared_ptr<Bone>> boneMap;

    void reg(std::shared_ptr<Bone> b){ boneMap[b->name]=b; }
};

HeadSkeleton buildHeadSkeleton(){
    HeadSkeleton h;

    // ── Neck chain ──
    h.cervical1 = std::make_shared<Bone>("Cervical1", 3.0, Vec3(0,130,0), -15,15);
    h.cervical2 = std::make_shared<Bone>("Cervical2", 3.0, Vec3(0,0,0),  -15,15);
    h.neck      = std::make_shared<Bone>("Neck",      6.0, Vec3(0,0,0),  -45,45);
    h.head      = std::make_shared<Bone>("Head",      10.0,Vec3(0,0,0),  -30,30);
    h.skull     = std::make_shared<Bone>("Skull",     5.0, Vec3(0,0,0),  -20,20);

    h.cervical1->addChild(h.cervical2);
    h.cervical2->addChild(h.neck);
    h.neck->addChild(h.head);
    h.head->addChild(h.skull);

    // ── Jaw ──
    h.jaw     = std::make_shared<Bone>("Jaw",     4.0, Vec3(0,-2, 2), -30, 5);
    h.chinTip = std::make_shared<Bone>("ChinTip", 2.0, Vec3(0, 0, 0),   0, 0);
    h.jaw->addChild(h.chinTip);
    h.head->addChild(h.jaw);

    // ── Eyes ──
    h.eyeLeft  = std::make_shared<Bone>("EyeLeft",  1.2, Vec3(-3, 3, 5), -45,45);
    h.eyeRight = std::make_shared<Bone>("EyeRight", 1.2, Vec3( 3, 3, 5), -45,45);
    h.head->addChild(h.eyeLeft);
    h.head->addChild(h.eyeRight);

    // ── Eyebrow bones ──
    h.browLeftInner  = std::make_shared<Bone>("BrowL_Inner", 0.5,Vec3(-1.5,5,4));
    h.browLeftMid    = std::make_shared<Bone>("BrowL_Mid",   0.5,Vec3(-2.5,5.5,4));
    h.browLeftOuter  = std::make_shared<Bone>("BrowL_Outer", 0.5,Vec3(-3.5,5,4));
    h.browRightInner = std::make_shared<Bone>("BrowR_Inner", 0.5,Vec3( 1.5,5,4));
    h.browRightMid   = std::make_shared<Bone>("BrowR_Mid",   0.5,Vec3( 2.5,5.5,4));
    h.browRightOuter = std::make_shared<Bone>("BrowR_Outer", 0.5,Vec3( 3.5,5,4));
    h.head->addChild(h.browLeftInner);
    h.head->addChild(h.browLeftMid);
    h.head->addChild(h.browLeftOuter);
    h.head->addChild(h.browRightInner);
    h.head->addChild(h.browRightMid);
    h.head->addChild(h.browRightOuter);

    // ── Nose ──
    h.noseRoot    = std::make_shared<Bone>("NoseRoot",    1.5, Vec3(0,1,6));
    h.noseTip     = std::make_shared<Bone>("NoseTip",     0.8, Vec3(0,0,0));
    h.nostrilLeft = std::make_shared<Bone>("NostrilLeft", 0.5, Vec3(-1,-0.5,0));
    h.nostrilRight= std::make_shared<Bone>("NostrilRight",0.5, Vec3( 1,-0.5,0));
    h.noseRoot->addChild(h.noseTip);
    h.noseRoot->addChild(h.nostrilLeft);
    h.noseRoot->addChild(h.nostrilRight);
    h.head->addChild(h.noseRoot);

    // ── Lips ──
    h.lipCornerLeft  = std::make_shared<Bone>("LipCornerLeft",  0.5, Vec3(-2,-1.5,5));
    h.lipCornerRight = std::make_shared<Bone>("LipCornerRight", 0.5, Vec3( 2,-1.5,5));
    h.lipUpperMid    = std::make_shared<Bone>("LipUpperMid",    0.5, Vec3( 0,-1,  5));
    h.lipLowerMid    = std::make_shared<Bone>("LipLowerMid",    0.5, Vec3( 0,-2,  5));
    h.head->addChild(h.lipCornerLeft);
    h.head->addChild(h.lipCornerRight);
    h.head->addChild(h.lipUpperMid);
    h.head->addChild(h.lipLowerMid);

    // ── Cheeks ──
    h.cheekLeft  = std::make_shared<Bone>("CheekLeft",  0.5, Vec3(-3.5,0,4));
    h.cheekRight = std::make_shared<Bone>("CheekRight", 0.5, Vec3( 3.5,0,4));
    h.head->addChild(h.cheekLeft);
    h.head->addChild(h.cheekRight);

    // ── Tongue ──
    h.tongueRoot = std::make_shared<Bone>("TongueRoot", 1.5, Vec3(0,-2,1), -30,30);
    h.tongueMid  = std::make_shared<Bone>("TongueMid",  1.5, Vec3(0,0,0),  -30,30);
    h.tongueTip  = std::make_shared<Bone>("TongueTip",  1.0, Vec3(0,0,0),  -45,45);
    h.tongueRoot->addChild(h.tongueMid);
    h.tongueMid->addChild(h.tongueTip);
    h.jaw->addChild(h.tongueRoot);

    // ── Ears ──
    h.earLeft  = std::make_shared<Bone>("EarLeft",  1.5, Vec3(-5,1,0));
    h.earRight = std::make_shared<Bone>("EarRight", 1.5, Vec3( 5,1,0));
    h.head->addChild(h.earLeft);
    h.head->addChild(h.earRight);

    // ── Register all bones ──
    for(auto* b : {&h.cervical1,&h.cervical2,&h.neck,&h.head,&h.skull,
                   &h.jaw,&h.chinTip,&h.eyeLeft,&h.eyeRight,
                   &h.browLeftInner,&h.browLeftMid,&h.browLeftOuter,
                   &h.browRightInner,&h.browRightMid,&h.browRightOuter,
                   &h.noseRoot,&h.noseTip,&h.nostrilLeft,&h.nostrilRight,
                   &h.lipCornerLeft,&h.lipCornerRight,
                   &h.lipUpperMid,&h.lipLowerMid,
                   &h.cheekLeft,&h.cheekRight,
                   &h.tongueRoot,&h.tongueMid,&h.tongueTip,
                   &h.earLeft,&h.earRight})
        h.reg(*b);

    // ── Hair strands (around scalp) ──
    std::vector<std::pair<std::string,Vec3>> hairRoots = {
        {"Hair_TopCenter",    Vec3( 0.0, 170, 2)},
        {"Hair_TopLeft",      Vec3(-2.5, 170, 1)},
        {"Hair_TopRight",     Vec3( 2.5, 170, 1)},
        {"Hair_FrontCenter",  Vec3( 0.0, 169, 5)},
        {"Hair_FrontLeft",    Vec3(-2.0, 168, 5)},
        {"Hair_FrontRight",   Vec3( 2.0, 168, 5)},
        {"Hair_SideLeft",     Vec3(-5.5, 165, 0)},
        {"Hair_SideRight",    Vec3( 5.5, 165, 0)},
        {"Hair_BackLeft",     Vec3(-3.0, 167,-3)},
        {"Hair_BackRight",    Vec3( 3.0, 167,-3)},
        {"Hair_BackCenter",   Vec3( 0.0, 167,-4)},
        {"Hair_NapeLeft",     Vec3(-2.0, 158,-3)},
        {"Hair_NapeRight",    Vec3( 2.0, 158,-3)},
    };

    for(auto& [name, root] : hairRoots)
        h.hairStrands.emplace_back(name, root, 7, 1.1, 0.25, 0.88);

    return h;
}

// ─────────────────────────────────────────────
//  KEYFRAME SYSTEM
// ─────────────────────────────────────────────

struct BonePose    { std::string name; Quat rotation; };
struct ShapePose   { std::string name; double weight; };

struct HeadKeyframe {
    double                 time;
    std::vector<BonePose>  bonePoses;
    std::vector<ShapePose> shapePoses;
};

class HeadAnimClip {
public:
    std::string                 name;
    double                      duration;
    std::vector<HeadKeyframe>   keyframes;

    HeadAnimClip(const std::string& n, double d): name(n), duration(d){}

    void addKeyframe(HeadKeyframe kf){
        keyframes.push_back(kf);
        std::sort(keyframes.begin(),keyframes.end(),
            [](auto& a,auto& b){return a.time<b.time;});
    }

    // Returns interpolated frame at time t
    HeadKeyframe sample(double t) const {
        t = std::fmod(t, duration);
        if(keyframes.empty()) return {};
        if(keyframes.size()==1) return keyframes[0];
        if(t <= keyframes.front().time) return keyframes.front();
        if(t >= keyframes.back().time)  return keyframes.back();

        for(size_t i=0;i+1<keyframes.size();++i){
            auto& a=keyframes[i]; auto& b=keyframes[i+1];
            if(t>=a.time && t<=b.time){
                double alpha=(t-a.time)/(b.time-a.time);
                HeadKeyframe out; out.time=t;
                for(size_t j=0;j<a.bonePoses.size()&&j<b.bonePoses.size();++j)
                    out.bonePoses.push_back({a.bonePoses[j].name,
                        Quat::slerp(a.bonePoses[j].rotation,
                                    b.bonePoses[j].rotation,alpha)});
                for(size_t j=0;j<a.shapePoses.size()&&j<b.shapePoses.size();++j)
                    out.shapePoses.push_back({a.shapePoses[j].name,
                        BlendShape::blend(a.shapePoses[j].weight,
                                          b.shapePoses[j].weight,alpha)});
                return out;
            }
        }
        return keyframes.back();
    }
};

// ─────────────────────────────────────────────
//  APPLY FRAME TO HEAD
// ─────────────────────────────────────────────

void applyHeadFrame(HeadSkeleton& h, const HeadKeyframe& frame){
    for(auto& bp : frame.bonePoses){
        auto it = h.boneMap.find(bp.name);
        if(it!=h.boneMap.end())
            it->second->localRotation = bp.rotation;
    }
    // Apply blend shapes by name
    auto setShape=[&](const std::string& n, double w){
        auto& f=h.facial;
        if(n=="BrowRaiseL")    f.browRaiseLeft.set(w);
        else if(n=="BrowRaiseR")   f.browRaiseRight.set(w);
        else if(n=="BrowFurrow")   f.browFurrow.set(w);
        else if(n=="BrowLowerL")   f.browLowerLeft.set(w);
        else if(n=="BrowLowerR")   f.browLowerRight.set(w);
        else if(n=="EyeSquintL")   f.eyeSquintLeft.set(w);
        else if(n=="EyeSquintR")   f.eyeSquintRight.set(w);
        else if(n=="EyeWideL")     f.eyeWideLeft.set(w);
        else if(n=="EyeWideR")     f.eyeWideRight.set(w);
        else if(n=="NoseSneer")    f.noseSneer.set(w);
        else if(n=="NostrilFlare") f.nostrilFlare.set(w);
        else if(n=="SmileL")       f.mouthSmileLeft.set(w);
        else if(n=="SmileR")       f.mouthSmileRight.set(w);
        else if(n=="FrownL")       f.mouthFrownLeft.set(w);
        else if(n=="FrownR")       f.mouthFrownRight.set(w);
        else if(n=="MouthOpen")    f.mouthOpen.set(w);
        else if(n=="MouthPucker")  f.mouthPucker.set(w);
        else if(n=="JawOpen")      f.jawOpen.set(w);
        else if(n=="TongueOut")    f.tongueOut.set(w);
        else if(n=="CheekPuffL")   f.cheekPuffLeft.set(w);
        else if(n=="CheekPuffR")   f.cheekPuffRight.set(w);
        else if(n=="CheekRaiseL")  f.cheekRaiseLeft.set(w);
        else if(n=="CheekRaiseR")  f.cheekRaiseRight.set(w);
    };
    for(auto& sp : frame.shapePoses) setShape(sp.name, sp.weight);
}

// ─────────────────────────────────────────────
//  PRESET EXPRESSIONS
// ─────────────────────────────────────────────

auto deg2rad=[](double d){ return d*PI/180.0; };
auto axisRot=[&](const std::string& bone, Vec3 axis, double deg) -> BonePose {
    return {bone, Quat::fromAxisAngle(axis, deg*PI/180.0)};
};

HeadKeyframe expressionNeutral(){
    HeadKeyframe kf; kf.time=0;
    // Bones — all identity
    for(auto& n : {"Neck","Head","Jaw","EyeLeft","EyeRight",
                   "TongueRoot","TongueMid","TongueTip"})
        kf.bonePoses.push_back({n, Quat()});
    return kf;
}

HeadKeyframe expressionHappy(){
    HeadKeyframe kf; kf.time=0;
    kf.bonePoses.push_back({"Head",Quat::fromAxisAngle({1,0,0}, 0.08)});
    kf.shapePoses={
        {"SmileL",0.9},{"SmileR",0.9},
        {"CheekRaiseL",0.7},{"CheekRaiseR",0.7},
        {"EyeSquintL",0.4},{"EyeSquintR",0.4},
        {"BrowRaiseL",0.3},{"BrowRaiseR",0.3},
        {"MouthOpen",0.2}
    };
    return kf;
}

HeadKeyframe expressionSad(){
    HeadKeyframe kf; kf.time=0;
    kf.bonePoses.push_back({"Head",Quat::fromAxisAngle({1,0,0},-0.1)});
    kf.shapePoses={
        {"FrownL",0.85},{"FrownR",0.85},
        {"BrowRaiseL",0.6},{"BrowRaiseR",0.6},
        {"BrowFurrow",0.5},
        {"EyeSquintL",0.3},{"EyeSquintR",0.3},
        {"MouthOpen",0.15}
    };
    return kf;
}

HeadKeyframe expressionAngry(){
    HeadKeyframe kf; kf.time=0;
    kf.bonePoses.push_back({"Head",Quat::fromAxisAngle({1,0,0},-0.12)});
    kf.shapePoses={
        {"BrowFurrow",1.0},
        {"BrowLowerL",0.8},{"BrowLowerR",0.8},
        {"EyeWideL",0.3},{"EyeWideR",0.3},
        {"NoseSneer",0.6},{"NostrilFlare",0.7},
        {"FrownL",0.5},{"FrownR",0.5},
        {"MouthOpen",0.1}
    };
    return kf;
}

HeadKeyframe expressionSurprised(){
    HeadKeyframe kf; kf.time=0;
    kf.bonePoses.push_back({"Head",Quat::fromAxisAngle({1,0,0}, 0.15)});
    kf.bonePoses.push_back({"Jaw", Quat::fromAxisAngle({1,0,0}, 0.4)});
    kf.shapePoses={
        {"BrowRaiseL",1.0},{"BrowRaiseR",1.0},
        {"EyeWideL",1.0},{"EyeWideR",1.0},
        {"MouthOpen",0.9},{"JawOpen",0.8}
    };
    return kf;
}

HeadKeyframe expressionDisgust(){
    HeadKeyframe kf; kf.time=0;
    kf.bonePoses.push_back({"Head",Quat::fromAxisAngle({0,1,0},-0.1)});
    kf.shapePoses={
        {"NoseSneer",1.0},{"NoseWrinkle",0.8},
        {"NostrilFlare",0.4},
        {"BrowFurrow",0.6},
        {"EyeSquintL",0.5},{"EyeSquintR",0.5},
        {"FrownL",0.4},{"FrownR",0.4},
        {"MouthOpen",0.05}
    };
    return kf;
}

HeadKeyframe expressionFear(){
    HeadKeyframe kf; kf.time=0;
    kf.bonePoses.push_back({"Head",Quat::fromAxisAngle({1,0,0},-0.05)});
    kf.shapePoses={
        {"BrowRaiseL",0.8},{"BrowRaiseR",0.8},
        {"BrowFurrow",0.6},
        {"EyeWideL",0.9},{"EyeWideR",0.9},
        {"MouthStretch",0.7},
        {"MouthOpen",0.4}
    };
    return kf;
}

HeadKeyframe expressionKiss(){
    HeadKeyframe kf; kf.time=0;
    kf.shapePoses={
        {"MouthPucker",1.0},
        {"CheekRaiseL",0.3},{"CheekRaiseR",0.3},
        {"SmileL",0.1},{"SmileR",0.1}
    };
    return kf;
}

HeadKeyframe expressionTongueOut(){
    HeadKeyframe kf; kf.time=0;
    kf.bonePoses.push_back({"TongueRoot",Quat::fromAxisAngle({1,0,0}, 0.2)});
    kf.bonePoses.push_back({"TongueMid", Quat::fromAxisAngle({1,0,0}, 0.3)});
    kf.bonePoses.push_back({"TongueTip", Quat::fromAxisAngle({1,0,0}, 0.15)});
    kf.shapePoses={
        {"MouthOpen",0.5},{"JawOpen",0.4},{"TongueOut",1.0}
    };
    return kf;
}

// ─────────────────────────────────────────────
//  ANIMATION CLIPS
// ─────────────────────────────────────────────

HeadAnimClip buildBlinkClip(){
    HeadAnimClip clip("Blink", 0.4);
    {   HeadKeyframe kf; kf.time=0.0;
        kf.shapePoses={{"EyeSquintL",0},{"EyeSquintR",0}};
        clip.addKeyframe(kf); }
    {   HeadKeyframe kf; kf.time=0.15;
        kf.shapePoses={{"EyeSquintL",1},{"EyeSquintR",1}};
        clip.addKeyframe(kf); }
    {   HeadKeyframe kf; kf.time=0.4;
        kf.shapePoses={{"EyeSquintL",0},{"EyeSquintR",0}};
        clip.addKeyframe(kf); }
    return clip;
}

HeadAnimClip buildNodClip(){
    HeadAnimClip clip("Nod", 1.0);
    {   HeadKeyframe kf; kf.time=0.0;
        kf.bonePoses.push_back({"Head",Quat()});
        clip.addKeyframe(kf); }
    {   HeadKeyframe kf; kf.time=0.25;
        kf.bonePoses.push_back({"Head",
            Quat::fromAxisAngle({1,0,0}, deg2rad(-20))});
        clip.addKeyframe(kf); }
    {   HeadKeyframe kf; kf.time=0.5;
        kf.bonePoses.push_back({"Head",Quat()});
        clip.addKeyframe(kf); }
    {   HeadKeyframe kf; kf.time=0.75;
        kf.bonePoses.push_back({"Head",
            Quat::fromAxisAngle({1,0,0}, deg2rad(-15))});
        clip.addKeyframe(kf); }
    {   HeadKeyframe kf; kf.time=1.0;
        kf.bonePoses.push_back({"Head",Quat()});
        clip.addKeyframe(kf); }
    return clip;
}

HeadAnimClip buildShakeClip(){
    HeadAnimClip clip("HeadShake", 1.0);
    auto addShake=[&](double t, double yDeg){
        HeadKeyframe kf; kf.time=t;
        kf.bonePoses.push_back({"Head",
            Quat::fromAxisAngle({0,1,0}, deg2rad(yDeg))});
        clip.addKeyframe(kf);
    };
    addShake(0.0,  0); addShake(0.2,-25);
    addShake(0.5, 25); addShake(0.8,-20);
    addShake(1.0,  0);
    return clip;
}

HeadAnimClip buildTalkClip(){
    HeadAnimClip clip("Talk", 1.6);
    double jawPattern[]={0,0.3,0.1,0.4,0.05,0.35,0.15,0.3,0};
    double times[]     ={0,0.2,0.4,0.6,0.8, 1.0, 1.2, 1.4,1.6};
    for(int i=0;i<9;++i){
        HeadKeyframe kf; kf.time=times[i];
        kf.bonePoses.push_back({"Jaw",
            Quat::fromAxisAngle({1,0,0}, deg2rad(jawPattern[i]*25))});
        kf.shapePoses={{"MouthOpen",jawPattern[i]},{"JawOpen",jawPattern[i]*0.7}};
        clip.addKeyframe(kf);
    }
    return clip;
}

HeadAnimClip buildLookAroundClip(){
    HeadAnimClip clip("LookAround", 3.0);
    struct LookPt { double t; double yaw; double pitch; };
    std::vector<LookPt> pts={
        {0.0,  0,  0},{0.5,-30, 5},{1.0,-30,-10},
        {1.5,  0,  0},{2.0, 30, 5},{2.5, 30,-10},{3.0,0,0}
    };
    for(auto& pt : pts){
        HeadKeyframe kf; kf.time=pt.t;
        Quat yaw  = Quat::fromAxisAngle({0,1,0}, deg2rad(pt.yaw));
        Quat pitch= Quat::fromAxisAngle({1,0,0}, deg2rad(pt.pitch));
        kf.bonePoses.push_back({"Head",(yaw*pitch).normalized()});
        clip.addKeyframe(kf);
    }
    return clip;
}

// ─────────────────────────────────────────────
//  ANIMATOR
// ─────────────────────────────────────────────

class HeadAnimator {
public:
    HeadSkeleton&   skeleton;
    HeadAnimClip*   clip    = nullptr;
    double          playhead= 0.0;
    double          speed   = 1.0;
    bool            playing = false;

    explicit HeadAnimator(HeadSkeleton& s): skeleton(s){}

    void play(HeadAnimClip& c, double spd=1.0){
        clip=&c; playhead=0; speed=spd; playing=true;
    }

    void update(double dt, Vec3 wind={0,0,0}){
        if(playing && clip){
            playhead += dt*speed;
            auto frame = clip->sample(playhead);
            applyHeadFrame(skeleton, frame);
            forwardKinematics(skeleton.cervical1,{0,0,0},Quat());
        }
        // Simulate hair physics
        Vec3 headTip = skeleton.head->worldTip;
        for(auto& strand : skeleton.hairStrands){
            strand.setRoot(strand.particles[0].position);
            strand.simulate(dt, {0,-9.8,0}, wind);
        }
    }
};

// ─────────────────────────────────────────────
//  PRINT UTILITIES
// ─────────────────────────────────────────────

void printBoneTree(std::shared_ptr<Bone> bone, int depth=0){
    std::string indent(depth*3,' ');
    std::cout << indent << "├─ [" << std::left<<std::setw(18)<<bone->name<<"]"
              << " len="<<std::fixed<<std::setprecision(1)<<bone->length
              << "  pos="; bone->worldPosition.print();
    std::cout<<"  tip="; bone->worldTip.print();
    std::cout<<"\n";
    for(auto& c:bone->children) printBoneTree(c,depth+1);
}

void printHeadState(HeadSkeleton& h){
    std::cout<<"\n── HEAD SKELETON ──\n";
    printBoneTree(h.cervical1);
    h.facial.print();
    std::cout<<"  ── Eyes ──\n";
    h.leftEye.print();
    h.rightEye.print();
    std::cout<<"  ── Hair (first 3 strands) ──\n";
    for(int i=0;i<3&&i<(int)h.hairStrands.size();++i)
        h.hairStrands[i].print();
}

void demoExpression(HeadSkeleton& h, const std::string& name,
                    HeadKeyframe expr){
    std::cout<<"\n┌─────────────────────────────────────────┐\n";
    std::cout<<"│  EXPRESSION: "<<std::left<<std::setw(27)<<name<<"│\n";
    std::cout<<"└─────────────────────────────────────────┘\n";
    h.facial.resetAll();
    applyHeadFrame(h, expr);
    forwardKinematics(h.cervical1,{0,0,0},Quat());
    h.facial.print();
}

void simulateHeadClip(HeadSkeleton& h, HeadAnimClip& clip,
                      int frames=8, double fps=24.0,
                      Vec3 wind={0,0,0}){
    std::cout<<"\n╔══════════════════════════════════════════╗\n";
    std::cout<<"║  CLIP: "<<std::left<<std::setw(34)<<clip.name<<"║\n";
    std::cout<<"╚══════════════════════════════════════════╝\n";
    HeadAnimator anim(h);
    anim.play(clip);
    double dt=1.0/fps;
    for(int f=0;f<frames;++f){
        anim.update(dt, wind);
        std::cout<<"\n── Frame "<<std::setw(3)<<f
                 <<"  (t="<<std::fixed<<std::setprecision(3)<<f*dt<<"s) ──\n";
        printBoneTree(h.cervical1);
        h.facial.print();
    }
}

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────

int main(){
    std::cout<<"╔══════════════════════════════════════════════╗\n";
    std::cout<<"║   CHARACTER HEAD & FACIAL ANIMATION SYSTEM   ║\n";
    std::cout<<"║   Bones + Blend Shapes + Hair Simulation      ║\n";
    std::cout<<"╚══════════════════════════════════════════════╝\n";

    HeadSkeleton head = buildHeadSkeleton();
    forwardKinematics(head.cervical1,{0,0,0},Quat());

    // ── Static expression demos ──
    demoExpression(head, "Neutral",   expressionNeutral());
    demoExpression(head, "Happy",     expressionHappy());
    demoExpression(head, "Sad",       expressionSad());
    demoExpression(head, "Angry",     expressionAngry());
    demoExpression(head, "Surprised", expressionSurprised());
    demoExpression(head, "Disgust",   expressionDisgust());
    demoExpression(head, "Fear",      expressionFear());
    demoExpression(head, "Kiss",      expressionKiss());
    demoExpression(head, "TongueOut", expressionTongueOut());

    // ── Eye controller demo ──
    std::cout<<"\n── EYE LOOK-AT DEMO ──\n";
    head.leftEye.lookTarget  = Vec3(-5, 158, 20);
    head.rightEye.lookTarget = Vec3(-5, 158, 20);
    head.leftEye.blink(0.0);
    head.rightEye.blink(0.0);
    head.leftEye.print();
    head.rightEye.print();

    // ── Animated clips ──
    auto blinkClip    = buildBlinkClip();
    auto nodClip      = buildNodClip();
    auto shakeClip    = buildShakeClip();
    auto talkClip     = buildTalkClip();
    auto lookClip     = buildLookAroundClip();

    simulateHeadClip(head, blinkClip,  6, 24);
    simulateHeadClip(head, nodClip,    8, 24);
    simulateHeadClip(head, shakeClip,  8, 24);
    simulateHeadClip(head, talkClip,   10,24);
    simulateHeadClip(head, lookClip,   10,24);

    // ── Hair wind simulation ──
    std::cout<<"\n╔══════════════════════════════════════════╗\n";
    std::cout<<"║  HAIR WIND PHYSICS SIMULATION             ║\n";
    std::cout<<"╚══════════════════════════════════════════╝\n";
    Vec3 wind{3.5, 0.5, 1.0};
    for(int f=0;f<6;++f){
        for(auto& strand : head.hairStrands)
            strand.simulate(1.0/24.0,{0,-9.8,0}, wind);
        std::cout<<"\nWind Frame "<<f<<":  wind=";
        wind.print(); std::cout<<"\n";
        if(f<3) head.hairStrands[0].print();
    }

    std::cout<<"\n✓ Head & Facial Animation simulation complete.\n";
    return 0;
}
