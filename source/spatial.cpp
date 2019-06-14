#include "spatial.hpp"

//-------------------------------------------------------------------------------------------------
Node*
Spatial::processor() { return qobject_cast<Node*>(m_processor); }

//-------------------------------------------------------------------------------------------------
void
Spatial::set_processor(Node* processor)
//-------------------------------------------------------------------------------------------------
{
    auto spp = qobject_cast<SpatialProcessor*>(processor);
    m_processor = spp;
    m_subnodes.push_front(processor);
    spp->add_input(this);
}
