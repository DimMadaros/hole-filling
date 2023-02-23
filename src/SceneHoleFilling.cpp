#include "SceneHoleFilling.h"

using namespace std;
using namespace vvr;

int main(int argc, char* argv[])
{
    try {
        return vvr::mainLoop(argc, argv, new HoleFillingScene);
    }
    catch (std::string exc) {
        cerr << exc << endl;
        return 1;
    }
    catch (...)
    {
        cerr << "Unknown exception" << endl;
        return 1;
    }
}

HoleFillingScene::HoleFillingScene()
{
    //! Load settings.
    vvr::Shape::DEF_LINE_WIDTH = 4;
    vvr::Shape::DEF_POINT_SIZE = 10;

    m_perspective_proj = true;

    // Set background and object colour
    m_bg_col = Colour("768E77");
    m_obj1_col = Colour("454545");
    m_obj2_col = Colour("454545");

    // Load objects
    const string objDir = getBasePath() + "resources/obj/";
    
    // Object 1
    const string objFile_1 = objDir + "polyhedron.obj";
    m_model_original_1 = vvr::Mesh(objFile_1);

    // Object 2
    const string objFile_2 = objDir + "polyhedron.obj";
    m_model_original_2 = vvr::Mesh(objFile_2);

    // Object 3
    const string objFile_3 = objDir + "armadillo_low_low.obj";
    m_model_original_3 = vvr::Mesh(objFile_3);

    // Set up initial object locations
    vec shift1(1.5, 0, 0);
    vec shift2(-1.5, 0, 0);
    vec shift3(-30, 0, 0);

    SetUp(m_model_original_1.getVertices(), shift1);
    SetUp(m_model_original_2.getVertices(), shift2);
    SetUp(m_model_original_3.getVertices(), shift3);

    reset();
}

void HoleFillingScene::reset()
{
    Scene::reset();

    PrintKeyboardShortcuts();

    //! Define what will be vissible by default
    m_style_flag = 0;
    m_style_flag &= FLAG_SHOW_AABB;
    m_style_flag &= FLAG_ERASE;
    m_style_flag &= FLAG_HIDE;
    m_style_flag |= FLAG_CHANGE_OBJ;
    m_style_flag |= FLAG_SHOW_TRIANGLES;
    m_style_flag |= FLAG_SHOW_SOLID;
    m_style_flag |= FLAG_SHOW_WIRE;

    // Set up initial object locations
    m_model_1 = m_model_original_1;
    m_model_2 = m_model_original_2;
    m_model_3 = m_model_original_3;

    CalcAABB(m_model_1.getVertices(), m_aabb_1);
    CalcAABB(m_model_2.getVertices(), m_aabb_2);
    CalcAABB(m_model_3.getVertices(), m_aabb_3);

    // Reset Control Variables
    areColliding = 0;
    readyPart2 = 0;
    keepObj = 0;
    disablePart1 = 0;
    disableHide = 0;
    boundaryCleaningFirstPass = 1;
    enable_model1_mov = 0;

    // Empty vectors
    hole_tris.clear();
    hole_edges.clear();
    sorted_hole_edges.clear();
    hole_indices.clear();
}

void HoleFillingScene::resize()
{
    //! By Making `first_pass` static and initializing it to true,
    //! we make sure that the if block will be executed only once.

    static bool first_pass = true;

    if (first_pass)
    {
        m_model_original_1.setBigSize(getSceneWidth() / 2);
        m_model_original_2.setBigSize(getSceneWidth() / 2);
        m_model_original_3.setBigSize(getSceneWidth() / 2);

        m_model_original_1.update();
        m_model_original_2.update();
        m_model_original_3.update();

        m_model_1 = m_model_original_1;
        m_model_2 = m_model_original_2;
        m_model_3 = m_model_original_3;

        CalcAABB(m_model_1.getVertices(), m_aabb_1);
        CalcAABB(m_model_2.getVertices(), m_aabb_2);
        CalcAABB(m_model_3.getVertices(), m_aabb_3);

        first_pass = false;
    }
}

void HoleFillingScene::arrowEvent(ArrowDir dir, int modif)
{
    // PART 2
    if (enable_model1_mov)
    {
        hole_tris.clear();
        hole_edges.clear();
        sorted_hole_edges.clear();
        hole_indices.clear();

        Displace(m_model_1.getVertices(), dir, shiftDown(modif));
        m_model_1.update();
    }
    // PART 1
    else
    {
        if (m_style_flag & FLAG_CHANGE_OBJ)
        {
            Displace(m_model_2.getVertices(), dir, shiftDown(modif));
            m_model_2.update();
            CalcAABB(m_model_2.getVertices(), m_aabb_2);
            areColliding = TestAABBs(m_aabb_1, m_aabb_2);
        }
        else
        {
            Displace(m_model_3.getVertices(), dir, shiftDown(modif));
            m_model_3.update();
            CalcAABB(m_model_3.getVertices(), m_aabb_3);
            areColliding = TestAABBs(m_aabb_1, m_aabb_3);
        }
    }
}

void HoleFillingScene::keyEvent(unsigned char key, bool up, int modif)
{
    Scene::keyEvent(key, up, modif);
    key = tolower(key);

    if (modif)
    {
        switch (key)
        {
        case 'h':
            if (!disableHide)
            {
                m_style_flag ^= FLAG_HIDE;
                keepObj = 2;
                enable_model1_mov = 1;
            }
            break;
        case '?': PrintKeyboardShortcuts(); break;
        }
    }
    else
    {
        switch (key)
        {
        case 'h':
            if (!disableHide)
            {
                m_style_flag ^= FLAG_HIDE;
                keepObj = 1;
                enable_model1_mov = 1;
            }
            break;
        case 'e': m_style_flag ^= FLAG_ERASE; break;
        case 't': m_style_flag ^= FLAG_SHOW_TRIANGLES; break;
        case 's': m_style_flag ^= FLAG_SHOW_SOLID; break;
        case 'w': m_style_flag ^= FLAG_SHOW_WIRE; break;
        case 'n': m_style_flag ^= FLAG_SHOW_NORMALS; break;
        case 'c':
            if (!disablePart1)
            {
                m_style_flag ^= FLAG_CHANGE_OBJ;
                m_model_1 = m_model_original_1;
                m_model_2 = m_model_original_2;
                m_model_3 = m_model_original_3;
                CalcAABB(m_model_1.getVertices(), m_aabb_1);
                CalcAABB(m_model_2.getVertices(), m_aabb_2);
                CalcAABB(m_model_3.getVertices(), m_aabb_3);
                areColliding = 0;
                readyPart2 = 0;
                keepObj = 0;
                disablePart1 = 0;
            }
            break;
        case 'b': m_style_flag ^= FLAG_SHOW_AABB; break;
        }
    }
}

void HoleFillingScene::draw()
{
    // PART 2
    if (readyPart2 == 1 && (m_style_flag & FLAG_HIDE))
    {
        disablePart1 = 1;
        disableHide = 1;

        // Draw chosen object
        if (keepObj == 1)
        {
            DrawSetup(m_model_1);

            if ((m_style_flag & FLAG_ERASE))
            {
                Cleaning(m_model_1.getTriangles(), boundaryCleaningFirstPass);
                boundaryCleaningFirstPass = 0;
            }

            if (boundaryCleaningFirstPass == 0)
            {
                FindHoleTriangles(m_model_1.getTriangles(), hole_tris, enable_model1_mov);
                FindHoleEdges(m_model_1.getTriangles(), hole_tris, hole_edges, enable_model1_mov);
                enable_model1_mov = 0;
            }
        }
        else if (keepObj == 2)
        {
            if (m_style_flag & FLAG_CHANGE_OBJ)
            {
                DrawSetup(m_model_2);

                if ((m_style_flag & FLAG_ERASE))
                {
                    Cleaning(m_model_2.getTriangles(), boundaryCleaningFirstPass);
                    boundaryCleaningFirstPass = 0;
                }

                if (boundaryCleaningFirstPass == 0)
                {
                    FindHoleTriangles(m_model_2.getTriangles(), hole_tris, enable_model1_mov);
                    FindHoleEdges(m_model_2.getTriangles(), hole_tris, hole_edges, enable_model1_mov);
                    enable_model1_mov = 0;
                }
            }

            else
            {
                DrawSetup(m_model_3);

                if ((m_style_flag & FLAG_ERASE))
                {
                    Cleaning(m_model_3.getTriangles(), boundaryCleaningFirstPass);
                    boundaryCleaningFirstPass = 0;
                }

                if (boundaryCleaningFirstPass == 0)
                {
                    FindHoleTriangles(m_model_3.getTriangles(), hole_tris, enable_model1_mov);
                    FindHoleEdges(m_model_3.getTriangles(), hole_tris, hole_edges, enable_model1_mov);
                    enable_model1_mov = 0;
                }
            }
        }

        //if (enable_model1_mov == 0) SortEdges(hole_edges, sorted_hole_edges, hole_indices);

        if (!(m_style_flag & FLAG_SHOW_TRIANGLES))
        {
            for (int i = 0; i < hole_edges.size(); i++)
            {
                hole_edges[i].draw();
            }
            ////cout << hole_indices.size() << endl;
            //for (int i = 0; i < hole_indices.size(); i++)
            //{
            //    cout << hole_indices[i] << endl;
            //}
        }


    }

    // PART 1
    else
    {
        if (!disablePart1)
        {
            // Draw chosen object
            DrawSetup(m_model_1);
            if (m_style_flag & FLAG_CHANGE_OBJ)
                DrawSetup(m_model_2);
            else
                DrawSetup(m_model_3);

            // Draw AABB
            if (m_style_flag & FLAG_SHOW_AABB)
            {
                DrawAABB(m_aabb_1, areColliding);

                if (m_style_flag & FLAG_CHANGE_OBJ)
                    DrawAABB(m_aabb_2, areColliding);
                else
                    DrawAABB(m_aabb_3, areColliding);
            }

            if (areColliding)
            {
                if (m_style_flag & FLAG_CHANGE_OBJ)
                {
                    if (TestTriangles(m_model_2.getTriangles(), m_model_1.getTriangles()))
                        readyPart2 = 1;
                }
                else
                {
                    if (TestTriangles(m_model_3.getTriangles(), m_model_1.getTriangles()))
                        readyPart2 = 1;
                }
            }
        }
    }      
}

void HoleFillingScene::PrintKeyboardShortcuts()
{
    std::cout << "Keyboard shortcuts:"
        << std::endl << "'?' => This shortcut list:"
        << std::endl
        << std::endl << "'b' => SHOW BOUNDING BOXES"
        << std::endl << "'r' => RESET"
        << std::endl << "'t' => SHOW INTERSECTING TRIANGLES"
        << std::endl << "'e' => ERASE INTERSECTING TRIANGLES (Press 2 Times)"
        << std::endl << "'c' => CHANGE INPUT MESH (left obj)"
        << std::endl << "'h' => HIDE LEFT OBJECT (only after intersecting triangles removal)"
        << std::endl << "'Shift + h' => HIDE RIGHT OBJECT (only after intersecting triangles removal)"
        << std::endl
        << std::endl << "!!AFTER HIDDING ONE OBJECT!!"
        << std::endl
        << std::endl << "'r' => RESET"
        << std::endl << "'t' => SHOW HOLE EDGES"
        << std::endl << "'e' => REMOVE 'TEETH' (Press 2 Times)"
        << std::endl
        << std::endl << "'!!ONLY FOR LEFT OBJECT UNTIL USE OF HIDE!!"
        << std::endl << "'!!FOR RIGHT OBJECT IF HIDE LEFT OBJECT UNTIL 'TEETH' REMOVAL!!"
        << std::endl << "'LEFT  ARROW'         => MOVE TO THE NEGATIVE OF X AXIS"
        << std::endl << "'RIGHT ARROW'         => MOVE TO THE POSITIVE OF X AXIS"
        << std::endl << "'DOWN  ARROW'         => MOVE TO THE NEGATIVE OF Y AXIS"
        << std::endl << "'UP    ARROW'         => MOVE TO THE POSITIVE OF Y AXIS"
        << std::endl << "'DOWN  ARROW + Shift' => MOVE TO THE NEGATIVE OF Z AXIS"
        << std::endl << "'UP    ARROW + Shift' => MOVE TO THE POSITIVE OF Z AXIS"
        << std::endl << std::endl;
}




// // // // // //
// Movement Related Functions
//

// Metatopizei to montelo kata orismenh metabolh
void SetUp(std::vector<vec>& vertices, const vec& shift)
{
    for (int i = 0; i < vertices.size(); i++)
        vertices[i] += shift;
}

// Metatopizei to montelo analoga me ta belakia pou exoun paty8ei
void Displace(std::vector<vec>& vertices, vvr::ArrowDir dir, int modif)
{
    vec disp(0, 0, 0);

    if (modif)
    {
        if (dir == UP)
        {
            disp.z -= 1;
            SetUp(vertices, disp);
        }

        else if (dir == DOWN)
        {
            disp.z += 1;
            SetUp(vertices, disp);
        }
    }

    else
    {
        if (dir == UP)
        {
            disp.y += 1;
            SetUp(vertices, disp);
        }

        else if (dir == DOWN)
        {
            disp.y -= 1;
            SetUp(vertices, disp);
        }

        else if (dir == LEFT)
        {
            disp.x -= 1;
            SetUp(vertices, disp);
        }

        else if (dir == RIGHT)
        {
            disp.x += 1;
            SetUp(vertices, disp);
        }
    }
}

//  Setarisma idiothtwn draw tou montelou 
void HoleFillingScene::DrawSetup(vvr::Mesh m_model)
{
    if (m_style_flag & FLAG_SHOW_SOLID)   m_model.draw(m_obj1_col, SOLID);
    if (m_style_flag & FLAG_SHOW_WIRE)    m_model.draw(Colour::black, WIRE);
    if (m_style_flag & FLAG_SHOW_NORMALS) m_model.draw(Colour::black, NORMALS);
}
// // // // // //




// // // // // //
// AABB Related Functions
//

// Ypologismos AABB
void CalcAABB(std::vector<vec>& vertices, vvr::Box3D &aabb)
{
    double max_x = vertices[0].x;
    double max_y = vertices[0].y;
    double max_z = vertices[0].z;
    double min_x = vertices[0].x;
    double min_y = vertices[0].y;
    double min_z = vertices[0].z;

    for (int i = 0; i < vertices.size(); i++)
    {
        if (vertices[i].x > max_x) max_x = vertices[i].x;
        if (vertices[i].y > max_y) max_y = vertices[i].y;
        if (vertices[i].z > max_z) max_z = vertices[i].z;

        if (vertices[i].x < min_x) min_x = vertices[i].x;
        if (vertices[i].y < min_y) min_y = vertices[i].y;
        if (vertices[i].z < min_z) min_z = vertices[i].z;
    }

    aabb.x1 = max_x;
    aabb.y1 = max_y;
    aabb.z1 = max_z;

    aabb.x2 = min_x;
    aabb.y2 = min_y;
    aabb.z2 = min_z;
}

// Draw AABB analoga me to an yparxei collision
void DrawAABB(vvr::Box3D m_aabb, int collide)
{
    if (collide) m_aabb.setColour(Colour::red);
    else m_aabb.setColour(Colour::black);

    m_aabb.setTransparency(1);
    m_aabb.draw();
}

// AABB collision Detection
int TestAABBs(vvr::Box3D a, vvr::Box3D b)
{
    // Den yparxei tomh an den yparxei epikalypsh se kapoion a3ona
    if (a.x1 < b.x2 || a.x2 > b.x1) return 0;
    if (a.y1 < b.y2 || a.y2 > b.y1) return 0;
    if (a.z1 < b.z2 || a.z2 > b.z1) return 0;

    // Tomh an yparxei epikalypsh kai stous 3 a3ones
    return 1;
}

//
// // // // // // 




// // // // // //
// Triangle Intersection, Colouring and Removal Related Functions
//

// Elegxos tomhs twn trigwnwn twn 2 montelwn
int TestTriangles(vector<vvr::Triangle>& tri1, vector<vvr::Triangle>& tri2)
{
    vector<vvr::Triangle> triangles1 = tri1;
    int isol = 0;

    for (int i = 0; i < tri1.size(); i++)
    {
        int intersects1 = 0;
        int intersects2 = 0;
        int erase = 0;
    
        for (int j = 0; j < tri2.size(); j++)
        {   
            intersects1 = TestTriTri(tri1[i], tri2[j]);
            intersects2 = TestTriTri(tri2[j], tri1[i]);

            if ((intersects1 || intersects2))
            {
                erase = 1;

                // Xrwmatismos temnomenwn trigwnwn
                if (m_style_flag & FLAG_SHOW_TRIANGLES)
                {
                    math2vvr(math::Triangle(tri1[i].v1(), tri1[i].v2(), tri1[i].v3()), vvr::Colour::darkGreen).draw();
                    math2vvr(math::Triangle(tri2[j].v1(), tri2[j].v2(), tri2[j].v3()), vvr::Colour::darkRed).draw();
                }
            }
        }

        // Afairesh tvn trigwnwn tou enos montelou
        if (erase && (m_style_flag & FLAG_ERASE))
        {
            tri1.erase(tri1.begin() + i);
            i--;  
            isol = 1;
        }
    }

    // Afairesh twn trigwnwn tou allou montelou me bash to
    // allo montelo prin thn afairesh twn trigwnwn tou
    for (int i = 0; i < tri2.size(); i++)
    {
        int intersects1 = 0;
        int intersects2 = 0;
        int erase = 0;

        for (int j = 0; j < triangles1.size(); j++)
        {
            intersects1 = TestTriTri(tri2[i], triangles1[j]);
            intersects2 = TestTriTri(triangles1[j], tri2[i]);

            if ((intersects1 || intersects2)) erase = 1;
        }

        if (erase && (m_style_flag & FLAG_ERASE))
        {
            tri2.erase(tri2.begin() + i);
            i--;
            isol = 1;
        }
    }

    if (isol) return 1;
    else return 0;
}

// Elegxos tomhs 2 trigwnwn
int TestTriTri(vvr::Triangle tri1, vvr::Triangle tri2)
{
    Plane plane1(tri1.v1(), tri1.v2(), tri1.v3());

    if (TestPlaneTriangle(tri2, plane1.normal, plane1.d) == 1)
    {
        LineSeg3D interLine = PlaneTriangleInter(tri2, plane1.normal, plane1.d);

        if (SegInTriangle(tri1, interLine)) return 1;
    }
    // Diaxeirish eidikhs periptwshs
    else if (TestPlaneTriangle(tri2, plane1.normal, plane1.d) == 2)
    {
        if (PointInTriangle(tri1, tri2.v1()) || PointInTriangle(tri1, tri2.v2()) || PointInTriangle(tri1, tri2.v3()))
            return 1;
        if (PointInTriangle(tri2, tri1.v1()) || PointInTriangle(tri2, tri1.v2()) || PointInTriangle(tri2, tri1.v3()))
            return 1;
    }

    return 0;
}

// Sxetikh 8esh trigwnou-epipedou
int TestPlaneTriangle(vvr::Triangle tri, vec n, float d)
{
    vec a = tri.v1();
    vec b = tri.v2();
    vec c = tri.v3();

    // ypologismos proshmasmenhs apostashs epipedou-koryfwn
    float distance1 = (a.Dot(n) - d);
    float distance2 = (b.Dot(n) - d);
    float distance3 = (c.Dot(n) - d);

    if (distance1 > 0 && distance2 > 0 && distance3 > 0) return 0;
    else if (distance1 < 0 && distance2 < 0 && distance3 < 0) return 0;
    else if (distance1 == 0 && distance2 == 0 && distance3 == 0) return 2;
    else return 1;
}

vvr::LineSeg3D PlaneTriangleInter(vvr::Triangle tri, vec n, float d)
{
    LineSeg3D interLine;

    vec a = tri.v1();
    vec b = tri.v2();
    vec c = tri.v3();

    int areInter1 = 0;
    int areInter2 = 0;
    int areInter3 = 0;

    // Ypologismos tou t gia ka8e pleyra pou temnei to epipedo
    vec ab = b - a;
    vec bc = c - b;
    vec ca = a - c;

    float t1 = (d - Dot(n, a)) / Dot(n, ab);
    float t2 = (d - Dot(n, b)) / Dot(n, bc);
    float t3 = (d - Dot(n, c)) / Dot(n, ca);

    // An t meta3y [0..1] ypologizoume to shmeio tomhs
    if (t1 >= 0.0f && t1 <= 1.0f) areInter1 = 1;
    if (t2 >= 0.0f && t2 <= 1.0f) areInter2 = 1;
    if (t3 >= 0.0f && t3 <= 1.0f) areInter3 = 1;

    if (areInter1 && areInter2)
    {
        vec p1 = a + t1 * ab;
        vec p2 = b + t2 * bc;

        LineSeg3D line(p1.x, p1.y, p1.z, p2.x, p2.y, p2.z);
        interLine = line;
    }
    else if (areInter2 && areInter3)
    {
        vec p2 = b + t2 * bc;
        vec p3 = c + t3 * ca;

        LineSeg3D line(p2.x, p2.y, p2.z, p3.x, p3.y, p3.z);
        interLine = line;
    }
    else if (areInter3 && areInter1)
    {
        vec p1 = a + t1 * ab;
        vec p3 = c + t3 * ca;

        LineSeg3D line(p3.x, p3.y, p3.z, p1.x, p1.y, p1.z);
        interLine = line;
    }

    return interLine;
}

// Elegxos an ena ey8ygrammo tmhma anhkei se trigwno
int SegInTriangle(vvr::Triangle tri, vvr::LineSeg3D line)
{
    vec p1(line.x1, line.y1, line.z1);
    vec p2(line.x2, line.y2, line.z2);

    // Elegxoyme an toulaxiston ena akro tou tmhmatos anhkei sto trigwno
    int check1 = PointInTriangle(tri, p1);
    int check2 = PointInTriangle(tri, p2);

    if (check1 || check2) return 1;

    return 0;
}

// Elegxos an ena shmeio anhkei sto trigwno mesw barycentrikwn syntetagmenwn
int PointInTriangle(vvr::Triangle tri, vec p)
{
    vec a = tri.v1();
    vec b = tri.v2();
    vec c = tri.v3();

    float u, v, w;// Barycentric coordinates

    vec v0 = b - a;
    vec v1 = c - a;
    vec v2 = p - a;

    float d00 = Dot(v0, v0);
    float d01 = Dot(v0, v1);
    float d11 = Dot(v1, v1);
    float d20 = Dot(v2, v0);
    float d21 = Dot(v2, v1);

    // Cramer
    float denom = d00 * d11 - d01 * d01;

    v = (d11 * d20 - d01 * d21) / denom;
    w = (d00 * d21 - d01 * d20) / denom;
    u = 1.0f - v - w;

    return (v >= 0.0f && w >= 0.0f && (v + w) <= 1.0f);
}
//
// // // // // //




// // // // // //
// Hole Finding and Cleaning Related Functions
//

// Elegxos gia isothta 2 vecs
int CheckVecs(vec v1, vec v2)
{
    if (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z) return 1;
    return 0;
}

// Metrhsh ari8mou geitonikvn trigwnwn
int CountAdjacentTriangles(vvr::Triangle t, vector<vvr::Triangle>& tris, int t_index)
{
    int count = 0;

    vec v1 = t.v1();
    vec v2 = t.v2();
    vec v3 = t.v3();

    for (int i = 0; i < tris.size(); i++)
    {
        if (i != t_index)
        {
            count += CheckEdgeOfTri(tris[i], v1, v2);
            count += CheckEdgeOfTri(tris[i], v2, v3);
            count += CheckEdgeOfTri(tris[i], v1, v3);
        }
    }

    return count;
}

// Elegxos an ena tmhma einai pleura trigwnou
int CheckEdgeOfTri(vvr::Triangle t, vec v1, vec v2)
{
    vec v_1 = t.v1();
    vec v_2 = t.v2();
    vec v_3 = t.v3();

    if (CheckVecs(v1, v_1) && CheckVecs(v2, v_2) || CheckVecs(v1, v_2) && CheckVecs(v2, v_1)) return 1;
    if (CheckVecs(v1, v_2) && CheckVecs(v2, v_3) || CheckVecs(v1, v_3) && CheckVecs(v2, v_2)) return 1;
    if (CheckVecs(v1, v_1) && CheckVecs(v2, v_3) || CheckVecs(v1, v_3) && CheckVecs(v2, v_1)) return 1;

    return 0;
}

// Afairesh teeth
int EraseTeeth(vector<vvr::Triangle>& tris)
{
    int checkAgain = 0;

    for (int i = 0; i < tris.size(); i++)
    {
        int count = CountAdjacentTriangles(tris[i], tris, i);

        if (count < 2)
        {
            tris.erase(tris.begin() + i);
            i--;
            checkAgain = 1;
        }
    }

    return checkAgain;
}

// Epanalhptikh afairesh teeth
void Cleaning(vector<vvr::Triangle>& tris, int once)
{
    if (once)
    {
        cout << "Finding and Cleaning Holes..." << endl;

        while (true)
        {
            int br = 0;
            br = EraseTeeth(tris);
            if (br == 0) break;
        }
    }
}

// Eyresh trigwnwn pou anhkoun se oph
void FindHoleTriangles(vector<vvr::Triangle>& tris, vector<vvr::Triangle>& holes, int once)
{
    if (once)
    {
        for (int i = 0; i < tris.size(); i++)
        {
            int count = CountAdjacentTriangles(tris[i], tris, i);

            if (count == 2) holes.push_back(tris[i]);
        }
    }
}

// Entopismos oriakwn perioxwn
void FindHoleEdges(vector<vvr::Triangle>& model, vector<vvr::Triangle>& holes, vector<vvr::LineSeg3D>& edges, int once)
{
    if (once)
    {
        for (int i = 0; i < holes.size(); i++)
        {
            vec v1 = holes[i].v1();
            vec v2 = holes[i].v2();
            vec v3 = holes[i].v3();

            int count1 = 0;
            int count2 = 0;
            int count3 = 0;

            int isAdj1 = 0;
            int isAdj2 = 0;
            int isAdj3 = 0;

            for (int j = 0; j < model.size(); j++)
            {
                isAdj1 = CheckEdgeOfTri(model[j], v1, v2);
                isAdj2 = CheckEdgeOfTri(model[j], v2, v3);
                isAdj3 = CheckEdgeOfTri(model[j], v1, v3);

                if (isAdj1 && isAdj2 && isAdj3) continue;

                if (!isAdj1) count1++;
                if (!isAdj2) count2++;
                if (!isAdj3) count3++;
            }

            if (count1 == (model.size() - 1))
            {
                LineSeg3D edge(v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, vvr::Colour::red);
                edges.push_back(edge);

            }
            if (count2 == (model.size() - 1))
            {
                LineSeg3D edge(v2.x, v2.y, v2.z, v3.x, v3.y, v3.z, vvr::Colour::red);
                edges.push_back(edge);
            }
            if (count3 == (model.size() - 1))
            {
                LineSeg3D edge(v1.x, v1.y, v1.z, v3.x, v3.y, v3.z, vvr::Colour::red);
                edges.push_back(edge);
            }
        }
        cout << "Finished!\n" << endl;
    }
}

// TODO:
// Sort gia na bre8ei kaue oph 3exwrista
void SortEdges(std::vector<vvr::LineSeg3D>& edges, std::vector<vvr::LineSeg3D>& sorted_edges, std::vector<int>& indices)
{
    vector<vvr::LineSeg3D> no_hole_edges;
    vector<vvr::LineSeg3D> edges_copy = edges;

    int track = -1;

    for (int i = 0; i < edges.size(); i++)
    {
  
        sorted_edges.push_back(edges[i]);
        track++;

        edges.erase(edges.begin() + i);
        i--;

        for (int j = 0; j < edges.size(); j++)
        {
            vec v1(sorted_edges[track].x1, sorted_edges[track].y1, sorted_edges[track].z1);
            vec v2(sorted_edges[track].x2, sorted_edges[track].y2, sorted_edges[track].z2);

            vec v_1(edges[j].x1, edges[j].y1, edges[j].z1);
            vec v_2(edges[j].x2, edges[j].y2, edges[j].z2);

            if (CheckVecs(v1, v_2))
            {
                sorted_edges.push_back(edges[j]);
                track++;

                edges.erase(edges.begin() + j);
                j = 0;
            }
            // Molis bre8ei kai to teleutaio edge ths ophs 8a paei sto epomeno i
        }

        vec v1(sorted_edges[0].x1, sorted_edges[0].y1, sorted_edges[0].z1);
        vec v2(sorted_edges[0].x2, sorted_edges[0].y2, sorted_edges[0].z2);
        vec v_1(sorted_edges[sorted_edges.size() - 1].x1, sorted_edges[sorted_edges.size() - 1].y1, sorted_edges[sorted_edges.size() - 1].z1);
        vec v_2(sorted_edges[sorted_edges.size() - 1].x2, sorted_edges[sorted_edges.size() - 1].y2, sorted_edges[sorted_edges.size() - 1].z2);
        if (CheckVecs(v2, v_1))
        {
            indices.push_back(sorted_edges.size() - 1);
        }

    }
}
//
// // // // // //