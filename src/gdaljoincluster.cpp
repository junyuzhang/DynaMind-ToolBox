#include "gdaljoincluster.h"

#include <ogr_api.h>
#include <ogrsf_frmts.h>
//#include <geos/operation/linemerge/LineMerger.h>
#include <geos_c.h>

#include <dm.h>
DM_DECLARE_NODE_NAME(GDALJoinCluster, GDALModules)


GDALJoinCluster::GDALJoinCluster()
{
	GDALModule = true;
	buffer = 0.5;
	this->addParameter("buffer", DM::DOUBLE, &buffer);
	network = DM::ViewContainer("network", DM::EDGE, DM::MODIFY);
	network.addAttribute("cluster_id",  DM::Attribute::INT, DM::READ);
	network.addAttribute("start_id",  DM::Attribute::INT, DM::MODIFY);
	network.addAttribute("end_id",  DM::Attribute::INT, DM::MODIFY);
	network.addAttribute("intersect_id",  DM::Attribute::INT, DM::WRITE);
	network.addAttribute("new",  DM::Attribute::INT, DM::WRITE);
	/*network.addAttribute("upstream_i", DM::Attribute::DOUBLE, DM::READ);
	network.addAttribute("downstre_1", DM::Attribute::DOUBLE, DM::READ);*/

	junctions = DM::ViewContainer("node", DM::NODE, DM::READ);
	junctions.addAttribute("node_id",  DM::Attribute::INT, DM::READ);
	junctions.addAttribute("possible_endpoint",  DM::Attribute::INT, DM::READ);
	junctions.addAttribute("intersects",  DM::Attribute::INT, DM::WRITE);
	std::vector<DM::ViewContainer*> data_stream;
	data_stream.push_back(&network);
	data_stream.push_back(&junctions);

	this->registerViewContainers(data_stream);
}

void GDALJoinCluster::run()
{
	junctions.createIndex("possible_endpoint");


	/*OGRFeature * f;
	junctions.resetReading();

	//edge_id, cluster id
	std::map<long, std::pair<long, long> > edge_list;
	std::map<long, std::vector<long> > start_nodes;
	std::map<long, int> edge_cluster;
	network.resetReading();

	//Init Node List
	while (f = network.getNextFeature()) {
		long start_id =f->GetFieldAsInteger("start_id");
		long end_id =f->GetFieldAsInteger("end_id");
		if (start_id == end_id)
			continue;
		edge_cluster[f->GetFID()] = -1;
		edge_list[f->GetFID()] = std::pair<long, long>(start_id, end_id);

		if (start_nodes.count(start_id) == 0)
			start_nodes[start_id] = std::vector<long>();

		if (start_nodes.count(end_id) == 0)
			start_nodes[end_id] = std::vector<long>();

		std::vector<long> & vec_start =  start_nodes[start_id];
		vec_start.push_back(f->GetFID());
		std::vector<long> & vec_end = start_nodes[end_id];
		vec_end.push_back(f->GetFID());

	}

	//Start point list
	std::vector<long> start;
	for (std::map<long,  std::vector<long> >::const_iterator it = start_nodes.begin(); it != start_nodes.end(); ++it) {

		if(it->second.size() == 1) {
			start.push_back(it->first);
		}
	}*/

	junctions.resetReading();
	//This should identify the endpoints
	//junctions.setAttributeFilter("possible_endpoint > 0");
	OGRFeature * f;

	typedef std::pair<long, double> segment;
	std::map<long, std::vector< segment > > segments;
	GEOSContextHandle_t gh = OGRGeometry::createGEOSContext();
	while (f = junctions.getNextFeature()) {
	//for(int i = 0; i < start.size(); i++) {
		//DM::Logger(DM::Debug) << i << "|" << start.size();
		//f = junctions.getFeature(start[i]);
		OGRPoint * p = (OGRPoint *)f->GetGeometryRef();
		if (!p)
			continue;
		OGRGeometry *p_buffer = p->Buffer(this->buffer);
		network.resetReading();
		network.setSpatialFilter(p_buffer);


		OGRFeature * f_n;
		int node_id = f->GetFieldAsInteger("node_id");

		while (f_n = network.getNextFeature()) {
			int start_node = f_n->GetFieldAsInteger("start_id");
			int end_node = f_n->GetFieldAsInteger("end_id");
			if (end_node == node_id || start_node == node_id)
				continue;
			OGRLineString * n = (OGRLineString *)f_n->GetGeometryRef();
			if (!n)
				continue;
			if (n->Intersects(p_buffer)) {
				GEOSGeometry* geos_p = p->exportToGEOS(gh);
				GEOSGeometry* line = n->exportToGEOS(gh);
				double p_l = GEOSProject_r(gh, line, geos_p);
				//DM::Logger(DM::Error) << p_l;
				if (p_l == 0) {
					continue;
				}
				if (segments.count(f_n->GetFID()) == 0) {
					std::vector<segment> seg_vec;
					seg_vec.push_back(segment(end_node, 0));
					seg_vec.push_back(segment(start_node, n->get_Length()));
					segments[f_n->GetFID()] = seg_vec;
				}
				std::vector<segment> & seg_vec = segments[f_n->GetFID()];
				seg_vec.push_back(segment(node_id, p_l));

				f_n->SetField("intersect_id", f->GetFieldAsInteger("node_id"));
				f->SetField("intersects", (int) f_n->GetFID());
			}
		}

	}

	this->network.syncReadFeatures();
	this->network.syncAlteredFeatures();
	this->junctions.syncReadFeatures();
	this->junctions.syncAlteredFeatures();

	for (std::map<long, std::vector< segment > >::const_iterator it = segments.begin(); it != segments.end(); ++it) {
		std::vector< segment > segements_vec = it->second;
		for(int i = 0; i < segements_vec.size(); i++) {
			segment s_i = segements_vec[i];
			//find min element
			int lowest_element = i;
			double current_low = s_i.second;
			for(int j = i; j < segements_vec.size(); j++) {
				segment s_j = segements_vec[j];
				if (s_j.second < current_low) {
					lowest_element = j;
					current_low = s_j.second;
				}

			}
			//switch pos
			segment s_tmp = segements_vec[lowest_element];
			segements_vec[i] = s_tmp;
			segements_vec[lowest_element] = s_i;
		}
		//Create Elements;
		long edge_id = it->first;
		OGRFeature * e = network.getFeature(edge_id);
		OGRLineString * n = (OGRLineString *)e->GetGeometryRef();
		for(int i = 1; i < segements_vec.size(); i++ ) {
			OGRLineString * s = n->getSubLine(segements_vec[i-1].second, segements_vec[i].second, false);
			//DM::Logger(DM::Error) << segements_vec[i-1].second << " " << segements_vec[i].second;
			if (!s) {
				DM::Logger(DM::Error) << segements_vec[i-1].second << " " << segements_vec[i].second;
				continue;
			}
			OGRFeature * s_f = this->network.createFeature();
			s_f->SetGeometry(s);
			s_f->SetField("new", 1);
			s_f->SetField("start_id", (int)segements_vec[i-1].first);
			s_f->SetField("end_id", (int)segements_vec[i].first);
		}
	}
}
