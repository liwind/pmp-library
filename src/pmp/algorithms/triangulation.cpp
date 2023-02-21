// Copyright 2011-2020 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#include "pmp/algorithms/triangulation.h"

#include <limits>

namespace pmp {

//! \brief Triangulate polygons to get a pure triangle mesh.
//! \details Triangulate n-gons into n-2 triangles. Finds the triangulation that
//! minimizes the sum of squared triangle areas, or the one that maximizes the
//! minimum angle.
//! See \cite liepa_2003_filling for details.
//! \ingroup algorithms
class Triangulation
{
public:
    //! Construct with mesh
    Triangulation(SurfaceMesh& mesh);

    //! Triangulate all faces
    void triangulate(
        TriangulationObjective o = TriangulationObjective::min_area);

    //! Triangulate the Face \p f
    //! \pre The input face is manifold
    //! \throw InvalidInputException in case the input precondition is violated
    void triangulate(
        Face f, TriangulationObjective o = TriangulationObjective::min_area);

private:
    // Compute the weight of the triangle (i,j,k).
    Scalar compute_weight(int i, int j, int k) const;

    // Does edge (a,b) exist?
    bool is_edge(Vertex a, Vertex b) const;

    // Add edge from vertex i to j.
    bool insert_edge(int i, int j);

    // mesh and properties
    SurfaceMesh& mesh_;
    VertexProperty<Point> points_;
    std::vector<Halfedge> halfedges_;
    std::vector<Vertex> vertices_;

    // data for computing optimal triangulation
    std::vector<std::vector<Scalar>> weight_;
    std::vector<std::vector<int>> index_;

    TriangulationObjective objective_;
};

Triangulation::Triangulation(SurfaceMesh& mesh) : mesh_(mesh)
{
    points_ = mesh_.vertex_property<Point>("v:point");
}

void Triangulation::triangulate(TriangulationObjective o)
{
    for (auto f : mesh_.faces())
        triangulate(f, o);
}

void Triangulation::triangulate(Face f, TriangulationObjective o)
{
    // store TriangulationObjective
    objective_ = o;

    // collect polygon halfedges
    Halfedge h0 = mesh_.halfedge(f);
    halfedges_.clear();
    vertices_.clear();
    Halfedge h = h0;
    do
    {
        if (!mesh_.is_manifold(mesh_.to_vertex(h)))
        {
            auto what = "[Triangulation] Non-manifold polygon";
            throw InvalidInputException(what);
        }

        halfedges_.emplace_back(h);
        vertices_.emplace_back(mesh_.to_vertex(h));
    } while ((h = mesh_.next_halfedge(h)) != h0);

    // do we have at least four vertices?
    const int n = halfedges_.size();
    if (n <= 3)
        return;

    // compute minimal triangulation by dynamic programming
    weight_.clear();
    weight_.resize(n,
                   std::vector<Scalar>(n, std::numeric_limits<Scalar>::max()));
    index_.clear();
    index_.resize(n, std::vector<int>(n, 0));

    int i, j, m, k, imin;
    Scalar w, wmin;

    // initialize 2-gons
    for (i = 0; i < n - 1; ++i)
    {
        weight_[i][i + 1] = 0.0;
        index_[i][i + 1] = -1;
    }

    // n-gons with n>2
    for (j = 2; j < n; ++j)
    {
        // for all n-gons [i,i+j]
        for (i = 0; i < n - j; ++i)
        {
            k = i + j;
            wmin = std::numeric_limits<Scalar>::max();
            imin = -1;

            // find best split i < m < i+j
            for (m = i + 1; m < k; ++m)
            {
                switch (objective_)
                {
                    case TriangulationObjective::min_area:
                        w = weight_[i][m] + compute_weight(i, m, k) +
                            weight_[m][k];
                        break;
                    case TriangulationObjective::max_angle:
                        w = std::max(
                            weight_[i][m],
                            std::max(compute_weight(i, m, k), weight_[m][k]));
                        break;
                    default:
                        // should never happen
                        exit(1);
                        break;
                }

                if (w < wmin)
                {
                    wmin = w;
                    imin = m;
                }
            }

            weight_[i][k] = wmin;
            index_[i][k] = imin;
        }
    }

    // now add triangles to mesh
    std::vector<ivec2> todo;
    todo.reserve(n);
    todo.emplace_back(0, n - 1);
    while (!todo.empty())
    {
        ivec2 tri = todo.back();
        todo.pop_back();
        int start = tri[0];
        int end = tri[1];
        if (end - start < 2)
            continue;
        int split = index_[start][end];

        insert_edge(start, split);
        insert_edge(split, end);

        todo.emplace_back(start, split);
        todo.emplace_back(split, end);
    }

    // clean up
    weight_.clear();
    index_.clear();
    halfedges_.clear();
    vertices_.clear();
}

Scalar Triangulation::compute_weight(int i, int j, int k) const
{
    const Vertex a = vertices_[i];
    const Vertex b = vertices_[j];
    const Vertex c = vertices_[k];

    // If one of the potential edges already exists this would result in an
    // invalid triangulation. This happens for suzanne.obj. Prevent this by
    // giving infinite weight.
    if (is_edge(a, b) && is_edge(b, c) && is_edge(c, a))
        return std::numeric_limits<Scalar>::max();

    const Point& pa = points_[a];
    const Point& pb = points_[b];
    const Point& pc = points_[c];

    Scalar w = std::numeric_limits<Scalar>::max();
    switch (objective_)
    {
        // compute squared triangle area
        case TriangulationObjective::min_area:
            w = sqrnorm(cross(pb - pa, pc - pa));
            break;

        // compute one over minimum angle
        // or cosine of minimum angle
        // maximum cosine (which should then be minimized)
        case TriangulationObjective::max_angle:
            Scalar cosa = dot(normalize(pb - pa), normalize(pc - pa));
            Scalar cosb = dot(normalize(pa - pb), normalize(pc - pb));
            Scalar cosc = dot(normalize(pa - pc), normalize(pb - pc));
            w = std::max(cosa, std::max(cosb, cosc));
            break;
    }

    return w;
}

bool Triangulation::is_edge(Vertex a, Vertex b) const
{
    return mesh_.find_halfedge(a, b).is_valid();
}

bool Triangulation::insert_edge(int i, int j)
{
    Halfedge h0 = halfedges_[i];
    Halfedge h1 = halfedges_[j];
    Vertex v0 = vertices_[i];
    Vertex v1 = vertices_[j];

    // does edge already exist?
    if (mesh_.find_halfedge(v0, v1).is_valid())
    {
        return false;
    }

    // can we reach v1 from h0?
    {
        Halfedge h = h0;
        do
        {
            h = mesh_.next_halfedge(h);
            if (mesh_.to_vertex(h) == v1)
            {
                mesh_.insert_edge(h0, h);
                return true;
            }
        } while (h != h0);
    }

    // can we reach v0 from h1?
    {
        Halfedge h = h1;
        do
        {
            h = mesh_.next_halfedge(h);
            if (mesh_.to_vertex(h) == v0)
            {
                mesh_.insert_edge(h1, h);
                return true;
            }
        } while (h != h1);
    }

    return false;
}

void triangulate(SurfaceMesh& mesh, TriangulationObjective o)
{
    Triangulation(mesh).triangulate(o);
}

void triangulate(SurfaceMesh& mesh, Face f, TriangulationObjective o)
{
    Triangulation(mesh).triangulate(f, o);
}

} // namespace pmp
