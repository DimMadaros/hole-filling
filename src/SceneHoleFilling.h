#include <VVRScene/canvas.h>
#include <VVRScene/mesh.h>
#include <VVRScene/settings.h>
#include <VVRScene/utils.h>
#include <MathGeoLib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <set>

#define FLAG_SHOW_TRIANGLES  1
#define FLAG_SHOW_WIRE       2
#define FLAG_SHOW_SOLID      4
#define FLAG_SHOW_NORMALS    8
#define FLAG_CHANGE_OBJ     16
#define FLAG_SHOW_AABB      32
#define FLAG_ERASE          64
#define FLAG_HIDE          128

// Metablhtes elegxou
int m_style_flag;
int readyPart2;
int keepObj;
int disablePart1;
int boundaryCleaningFirstPass;
int disableHide;
int enable_model1_mov;
int holeFirstPass;

// Synarthseis ylopoihshs project
void SetUp(std::vector<vec>& vertices, const vec& shift);
void Displace(std::vector<vec>& vertices, vvr::ArrowDir dir, int modif);
void CalcAABB(std::vector<vec>& vertices, vvr::Box3D& aabb);
void DrawAABB(vvr::Box3D m_aabb, int collide);
int TestAABBs(vvr::Box3D aabb1, vvr::Box3D aabb2);
int TestTriangles(std::vector<vvr::Triangle>& tri1, std::vector<vvr::Triangle>& tri2);
int TestPlaneTriangle(vvr::Triangle tri, vec n, float d);
vvr::LineSeg3D PlaneTriangleInter(vvr::Triangle tri, vec n, float d);
int PointInTriangle(vvr::Triangle tri, vec p);
int SegInTriangle(vvr::Triangle tri, vvr::LineSeg3D line);
int TestTriTri(vvr::Triangle tri1, vvr::Triangle tri2);
int CheckVecs(vec v1, vec v2);
int CheckEdgeOfTri(vvr::Triangle t, vec v1, vec v2);
int CountAdjacentTriangles(vvr::Triangle t, std::vector<vvr::Triangle>& tris, int t_index);
int EraseTeeth(std::vector<vvr::Triangle>& tris);
void Cleaning(std::vector<vvr::Triangle>& tris, int once);
void FindHoleTriangles(std::vector<vvr::Triangle>& tris, std::vector<vvr::Triangle>& holes, int once);
void FindHoleEdges(std::vector<vvr::Triangle>& model, std::vector<vvr::Triangle>& holes, std::vector<vvr::LineSeg3D>& edges, int once);
void SortEdges(std::vector<vvr::LineSeg3D>& edges, std::vector<vvr::LineSeg3D>& sorted_edges, std::vector<int>& indices);

class HoleFillingScene : public vvr::Scene
{
public:
    HoleFillingScene();
    const char* getName() const { return "Hole Filling"; }
    void keyEvent(unsigned char key, bool up, int modif) override;
    void arrowEvent(vvr::ArrowDir dir, int modif) override;

    // Synarthseis ylopoihshs project
    void DrawSetup(vvr::Mesh m_model);
    void PrintKeyboardShortcuts();

private:
    void draw() override;
    void reset() override;
    void resize() override;

private:
    int areColliding;
    vvr::Colour m_obj1_col, m_obj2_col;
    vvr::Mesh m_model_original_1, m_model_1;
    vvr::Mesh m_model_original_2, m_model_2;
    vvr::Mesh m_model_original_3, m_model_3;
    vvr::Box3D m_aabb_1, m_aabb_2, m_aabb_3;
    std::vector<vvr::Triangle> hole_tris;
    std::vector<vvr::LineSeg3D> hole_edges;
    std::vector<vvr::LineSeg3D> sorted_hole_edges;
    std::vector<int> hole_indices;
};