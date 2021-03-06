#include <limits>

#include "PolytopeSurface.h"
#include "Polytope.h"

namespace urchin
{
    PolytopeSurface::PolytopeSurface() :
            polytope(nullptr),
            surfacePosition(std::numeric_limits<unsigned int>::max()),
            walkableCandidate(true)
    {

    }

    void PolytopeSurface::setPolytope(const Polytope *polytope)
    {
        this->polytope = polytope;
        this->surfacePosition = computeSurfacePosition();
    }

    std::size_t PolytopeSurface::computeSurfacePosition()
    {
        for(std::size_t i=0; i<polytope->getSurfaces().size(); ++i)
        {
            if(polytope->getSurface(i).get()==this)
            {
                return i;
            }
        }

        throw std::runtime_error("Impossible to find surface position for polytope " + polytope->getName());
    }

    const Polytope *PolytopeSurface::getPolytope() const
    {
        return polytope;
    }

    std::size_t PolytopeSurface::getSurfacePosition() const
    {
        return surfacePosition;
    }

    void PolytopeSurface::setWalkableCandidate(bool walkableCandidate)
    {
        this->walkableCandidate = walkableCandidate;
    }

    bool PolytopeSurface::isWalkableCandidate() const
    {
        return walkableCandidate;
    }
}
