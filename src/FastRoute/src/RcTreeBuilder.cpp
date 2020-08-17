/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "RcTreeBuilder.h"

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "openroad/OpenRoad.hh"
#include "openroad/Error.hh"
#include "sta/Corner.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/Sdc.hh"
#include "sta/Units.hh"

namespace FastRoute {

using std::abs;
using std::min;

RcTreeBuilder::RcTreeBuilder(ord::OpenRoad* openroad,
			     DBWrapper* dbWrapper,
			     Grid* grid)
{
  _dbWrapper = dbWrapper;
  _grid = grid;
  _sta = openroad->getSta();
  _parasitics = _sta->parasitics();
  _corner = _sta->cmdCorner();
  _min_max = sta::MinMax::max();
  _analysisPoint = _corner->findParasiticAnalysisPt(_min_max);

  _network = openroad->getDbNetwork();
  _debug = false;
}

void RcTreeBuilder::estimateParasitcs(Net* net,
				      std::vector<ROUTE>& routes)
{
  if (_debug)
    printf("net %s\n", net->getDbNet()->getConstName());
  _net = net;
  _sta_net = _network->dbToSta(_net->getDbNet());
  _node_id = 0;
  _node_map.clear();

  _parasitic = _parasitics->makeParasiticNetwork(_sta_net, false,
						 _analysisPoint);
  makeRoutePtMap();
  makeRouteParasitics(routes);
  reduceParasiticNetwork();
}

RoutePt::RoutePt(int x,  int y, int layer) :
  _x(x),
  _y(y),
  _layer(layer)
{
}

bool operator<(const RoutePt &p1,
	       const RoutePt &p2)
{
  return (p1._x < p2._x)
    || (p1._x == p2._x
	&& p1._y < p2._y)
    || (p1._x == p2._x
	&& p1._y == p2._y
	&& p1._layer < p2._layer);
}

void RcTreeBuilder::makeRoutePtMap()
{
  for (Pin& pin : _net->getPins()) {
    const Coordinate& pt = pin.getPosition();
    int layer = pin.getTopLayer();
    RoutePt loc(pt.getX(), pt.getY(), layer);
    sta::Pin* sta_pin = staPin(pin);
    sta::ParasiticNode *pin_node = _parasitics->ensureParasiticNode(_parasitic, sta_pin);
    _node_map[loc] = pin_node;
    makeParasiticsToGrid(pin, pin_node);
  }
}

// Make parasitics for the wire from the pin to the grid location of the route.
void RcTreeBuilder::makeParasiticsToGrid(Pin& pin,
					 sta::ParasiticNode *pin_node)
{
  const Coordinate& grid_pt = pin.getOnGridPosition();
  sta::ParasiticNode *grid_node =
    _parasitics->ensureParasiticNode(_parasitic, _sta_net, _node_id++);
  int layer = pin.getTopLayer();
  RoutePt loc(grid_pt.getX(), grid_pt.getY(), layer);
  _node_map[loc] = grid_node;

  const Coordinate& pt = pin.getPosition();
  int wire_length_dbu = abs(pt.getX() - grid_pt.getX())
    + abs(pt.getY() - grid_pt.getY());
  float res, cap;
  layerRC(wire_length_dbu, layer, res, cap);

  _parasitics->incrCap(pin_node, cap / 2.0, _analysisPoint);
  _parasitics->makeResistor(nullptr, pin_node, grid_node, res, _analysisPoint);
  _parasitics->incrCap(grid_node, cap / 2.0, _analysisPoint);
}

sta::Pin* RcTreeBuilder::staPin(Pin& pin)
{
  if (pin.isPort())
    return _network->dbToSta(pin.getBTerm());
  else
    return _network->dbToSta(pin.getITerm());
}

void RcTreeBuilder::makeRouteParasitics(std::vector<ROUTE>& routes)
{
  for (ROUTE& route : routes) {
    sta::ParasiticNode *n1 = ensureParasiticNode(route.initX,
						 route.initY,
						 route.initLayer);
    sta::ParasiticNode *n2 = ensureParasiticNode(route.finalX,
						 route.finalY,
						 route.finalLayer);
    int wire_length_dbu = abs(route.initX - route.finalX)
      + abs(route.initY - route.finalY);
    float res, cap;
    if (wire_length_dbu == 0) {
      // via
      int lower_layer = min(route.initLayer, route.finalLayer);
      _dbWrapper->getCutLayerRes(lower_layer, res);
      cap = 0.0;
    }
    else if (route.initLayer == route.finalLayer)
      layerRC(wire_length_dbu, route.initLayer, res, cap);
    else
      ord::warn("non wire or via route found on net %s",
		_net->getDbNet()->getConstName());

    if (_debug) {
      sta::Units *units = _sta->units();
      if (_debug)
	printf("%s -> %s r=%s c=%s\n",
	       _parasitics->name(n1),
	       _parasitics->name(n2),
	       units->resistanceUnit()->asString(res),
	       units->capacitanceUnit()->asString(cap));
    }
    _parasitics->incrCap(n1, cap / 2.0, _analysisPoint);
    _parasitics->makeResistor(nullptr, n1, n2, res, _analysisPoint);
    _parasitics->incrCap(n2, cap / 2.0, _analysisPoint);
  }
}

void RcTreeBuilder::layerRC(int wire_length_dbu,
			    int layer,
			    // Return values.
			    float &res,
			    float &cap)
{
  float r_per_meter, cap_per_meter;
  _dbWrapper->getLayerRC(layer, r_per_meter, cap_per_meter);
  float wire_length = _dbWrapper->dbuToMeters(wire_length_dbu);
  res = r_per_meter * wire_length;
  cap = cap_per_meter * wire_length;
}

sta::ParasiticNode *RcTreeBuilder::ensureParasiticNode(int x,
						       int y,
						       int layer)
{
  RoutePt pin_loc(x,  y, layer);
  sta::ParasiticNode* node = _node_map[pin_loc];
  if (node == nullptr) {
    node = _parasitics->ensureParasiticNode(_parasitic, _sta_net, _node_id++);
    _node_map[pin_loc] = node;
  }
  return node;
}

void RcTreeBuilder::reduceParasiticNetwork()
{
  sta::Sdc* sdc = _sta->sdc();
  sta::OperatingConditions* op_cond = sdc->operatingConditions(_min_max);

  sta::ReduceParasiticsTo reduce_to = sta::ReduceParasiticsTo::pi_elmore;
  _parasitics->reduceTo(_parasitic, _sta_net, reduce_to, op_cond, _corner,
                        _min_max, _analysisPoint);
  _parasitics->deleteParasiticNetwork(_sta_net, _analysisPoint);
}

}  // namespace FastRoute
