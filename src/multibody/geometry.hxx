//
// Copyright (c) 2015-2021 CNRS INRIA
//

#ifndef __pinocchio_multibody_geometry_hxx__
#define __pinocchio_multibody_geometry_hxx__

#include <algorithm>

#include "pinocchio/multibody/model.hpp"

#if BOOST_VERSION / 100 % 1000 >= 60
  #include <boost/bind/bind.hpp>
  #include <boost/utility.hpp>
#else
  #include <boost/bind.hpp>
#endif

/// @cond DEV

namespace pinocchio
{

  inline GeometryData::GeometryData(const GeometryModel & geom_model)
  : oMg(geom_model.ngeoms)
  , activeCollisionPairs(geom_model.collisionPairs.size(), true)
#ifdef PINOCCHIO_WITH_HPP_FCL
  , distanceRequests(geom_model.collisionPairs.size(), hpp::fcl::DistanceRequest(true))
  , distanceResults(geom_model.collisionPairs.size())
  , collisionRequests(geom_model.collisionPairs.size(), hpp::fcl::CollisionRequest(::hpp::fcl::NO_REQUEST,1))
  , collisionResults(geom_model.collisionPairs.size())
  , radius()
  , collisionPairIndex(0)
#endif // PINOCCHIO_WITH_HPP_FCL
  , innerObjects()
  , outerObjects()
  {
#ifdef PINOCCHIO_WITH_HPP_FCL
    BOOST_FOREACH(hpp::fcl::CollisionRequest & creq, collisionRequests)
    {
      creq.enable_cached_gjk_guess = true;
    }
#if HPP_FCL_VERSION_AT_LEAST(1, 4, 5)
    BOOST_FOREACH(hpp::fcl::DistanceRequest & dreq, distanceRequests)
    {
      dreq.enable_cached_gjk_guess = true;
    }
#endif
#endif
    fillInnerOuterObjectMaps(geom_model);
  }

  inline GeometryData::GeometryData(const GeometryData & other)
  : oMg (other.oMg)
  , activeCollisionPairs (other.activeCollisionPairs)
#ifdef PINOCCHIO_WITH_HPP_FCL
  , distanceRequests (other.distanceRequests)
  , distanceResults (other.distanceResults)
  , collisionRequests (other.collisionRequests)
  , collisionResults (other.collisionResults)
  , radius (other.radius)
  , collisionPairIndex (other.collisionPairIndex)
#endif // PINOCCHIO_WITH_HPP_FCL
  , innerObjects (other.innerObjects)
  , outerObjects (other.outerObjects)
  {}

  inline GeometryData::~GeometryData() {}

  template<typename S2, int O2, template<typename,int> class JointCollectionTpl>
  GeomIndex GeometryModel::addGeometryObject(const GeometryObject & object,
                                             const ModelTpl<S2,O2,JointCollectionTpl> & model)
  {
    if(object.parentFrame < (FrameIndex)model.nframes)
      PINOCCHIO_CHECK_INPUT_ARGUMENT(model.frames[object.parentFrame].parent == object.parentJoint,
                                     "The object joint parent and its frame joint parent do not match.");
    
    GeomIndex idx = (GeomIndex) (ngeoms ++);
    geometryObjects.push_back(object);
    geometryObjects.back().parentJoint = model.frames[object.parentFrame].parent;
    return idx;
  }
  
  inline GeomIndex GeometryModel::addGeometryObject(const GeometryObject & object)
  {
    GeomIndex idx = (GeomIndex) (ngeoms ++);
    geometryObjects.push_back(object);
    return idx;
  }

  inline GeomIndex GeometryModel::getGeometryId(const std::string & name) const
  {
#if BOOST_VERSION / 100 % 1000 >= 60
    using namespace boost::placeholders;
#endif
    GeometryObjectVector::const_iterator it
    = std::find_if(geometryObjects.begin(),
                   geometryObjects.end(),
                   boost::bind(&GeometryObject::name, _1) == name
                   );
    return GeomIndex(it - geometryObjects.begin());
  }

  inline bool GeometryModel::existGeometryName(const std::string & name) const
  {
#if BOOST_VERSION / 100 % 1000 >= 60
    using namespace boost::placeholders;
#endif
    return std::find_if(geometryObjects.begin(),
                        geometryObjects.end(),
                        boost::bind(&GeometryObject::name, _1) == name) != geometryObjects.end();
  }

  inline void GeometryData::fillInnerOuterObjectMaps(const GeometryModel & geomModel)
  {
    innerObjects.clear();
    outerObjects.clear();

    for( GeomIndex gid = 0; gid<geomModel.geometryObjects.size(); gid++)
      innerObjects[geomModel.geometryObjects[gid].parentJoint].push_back(gid);

    BOOST_FOREACH(const CollisionPair & pair, geomModel.collisionPairs)
    {
      outerObjects[geomModel.geometryObjects[pair.first].parentJoint].push_back(pair.second);
    }
  }

  inline std::ostream & operator<< (std::ostream & os, const GeometryModel & geomModel)
  {
    os << "Nb geometry objects = " << geomModel.ngeoms << std::endl;
    
    for(GeomIndex i=0;i<(GeomIndex)(geomModel.ngeoms);++i)
    {
      os  << geomModel.geometryObjects[i] <<std::endl;
    }

    return os;
  }

  inline std::ostream & operator<< (std::ostream & os, const GeometryData & geomData)
  {
#ifdef PINOCCHIO_WITH_HPP_FCL
    os << "Number of collision pairs = " << geomData.activeCollisionPairs.size() << std::endl;
    
    for(PairIndex i=0;i<(PairIndex)(geomData.activeCollisionPairs.size());++i)
    {
      os << "Pairs " << i << (geomData.activeCollisionPairs[i]?" active":" inactive") << std::endl;
    }
#else
    os << "WARNING** Without fcl library, no collision checking or distance computations are possible. Only geometry placements can be computed." << std::endl;
    os << "Number of geometry objects = " << geomData.oMg.size() << std::endl;
#endif

    return os;
  }

  inline void GeometryModel::addCollisionPair(const CollisionPair & pair)
  {
    PINOCCHIO_CHECK_INPUT_ARGUMENT(pair.first < ngeoms,
                                   "The input pair.first is larger than the number of geometries contained in the GeometryModel");
    PINOCCHIO_CHECK_INPUT_ARGUMENT(pair.second < ngeoms,
                                   "The input pair.second is larger than the number of geometries contained in the GeometryModel");
    if (!existCollisionPair(pair)) { collisionPairs.push_back(pair); }
  }

  inline void GeometryModel::addCollisionPairs(const MatrixXb & map,
                                               const bool upper)
  {
    PINOCCHIO_CHECK_ARGUMENT_SIZE(map.rows(),(Eigen::DenseIndex)ngeoms,
                                  "Input map does not have the correct number of rows.");
    PINOCCHIO_CHECK_ARGUMENT_SIZE(map.cols(),(Eigen::DenseIndex)ngeoms,
                                  "Input map does not have the correct number of columns.");
    removeAllCollisionPairs();
    for(Eigen::DenseIndex i = 0; i < (Eigen::DenseIndex)ngeoms; ++i)
    {
      for(Eigen::DenseIndex j = i+1; j < (Eigen::DenseIndex)ngeoms; ++j)
      {
        if(upper)
        {
          if(map(i,j))
            collisionPairs.push_back(CollisionPair((std::size_t)i,(std::size_t)j));
        }
        else
        {
          if(map(j,i))
            collisionPairs.push_back(CollisionPair((std::size_t)i,(std::size_t)j));
        }
      }
    }
  }
  
  inline void GeometryModel::addAllCollisionPairs()
  {
    removeAllCollisionPairs();
    for (GeomIndex i = 0; i < ngeoms; ++i)
    {
      const JointIndex joint_i = geometryObjects[i].parentJoint;
      for (GeomIndex j = i+1; j < ngeoms; ++j)
      {
        const JointIndex joint_j = geometryObjects[j].parentJoint;
        if (joint_i != joint_j)
          addCollisionPair(CollisionPair(i,j));
      }
    }
  }
  
  inline void GeometryModel::removeCollisionPair(const CollisionPair & pair)
  {
    PINOCCHIO_CHECK_INPUT_ARGUMENT(pair.first < ngeoms,
                                   "The input pair.first is larger than the number of geometries contained in the GeometryModel");
    PINOCCHIO_CHECK_INPUT_ARGUMENT(pair.second < ngeoms,
                                   "The input pair.second is larger than the number of geometries contained in the GeometryModel");

    CollisionPairVector::iterator it = std::find(collisionPairs.begin(),
                                                 collisionPairs.end(),
                                                 pair);
    if (it != collisionPairs.end()) { collisionPairs.erase(it); }
  }
  
  inline void GeometryModel::removeAllCollisionPairs() { collisionPairs.clear(); }

  inline bool GeometryModel::existCollisionPair(const CollisionPair & pair) const
  {
    return (std::find(collisionPairs.begin(),
                      collisionPairs.end(),
                      pair) != collisionPairs.end());
  }
  
  inline PairIndex GeometryModel::findCollisionPair(const CollisionPair & pair) const
  {
    CollisionPairVector::const_iterator it = std::find(collisionPairs.begin(),
                                                       collisionPairs.end(),
                                                       pair);
    
    return (PairIndex) std::distance(collisionPairs.begin(), it);
  }
  
  inline void GeometryData::activateCollisionPair(const PairIndex pairId)
  {
    PINOCCHIO_CHECK_INPUT_ARGUMENT(pairId < activeCollisionPairs.size(),
                                   "The input argument pairId is larger than the number of collision pairs contained in activeCollisionPairs.");
    activeCollisionPairs[pairId] = true;
  }

  inline void GeometryData::activateAllCollisionPairs()
  {
    std::fill(activeCollisionPairs.begin(),activeCollisionPairs.end(),true);
  }

  inline void GeometryData::setActiveCollisionPairs(const GeometryModel & geom_model,
                                                    const MatrixXb & map,
                                                    const bool upper)
  {
    const Eigen::DenseIndex ngeoms = (Eigen::DenseIndex)geom_model.ngeoms;
    PINOCCHIO_CHECK_ARGUMENT_SIZE(map.rows(),ngeoms,"Input map does not have the correct number of rows.");
    PINOCCHIO_CHECK_ARGUMENT_SIZE(map.cols(),ngeoms,"Input map does not have the correct number of columns.");
    PINOCCHIO_CHECK_ARGUMENT_SIZE(geom_model.collisionPairs.size(),activeCollisionPairs.size(),"Current geometry data and the input geometry model are not conistent.");
    
    for(size_t k = 0; k < geom_model.collisionPairs.size(); ++k)
    {
      const CollisionPair & cp = geom_model.collisionPairs[k];
      
      Eigen::DenseIndex i,j;
      if(upper)
      {
        j = (Eigen::DenseIndex)std::max(cp.first,cp.second);
        i = (Eigen::DenseIndex)std::min(cp.first,cp.second);
      }
      else
      {
        i = (Eigen::DenseIndex)std::max(cp.first,cp.second);
        j = (Eigen::DenseIndex)std::min(cp.first,cp.second);
      }
      
      activeCollisionPairs[k] = map(i,j);
    }
  }

  inline void GeometryData::setSecurityMargins(const GeometryModel & geom_model,
                                               const MatrixXs & security_margin_map,
                                               const bool upper)
  {
    const Eigen::DenseIndex ngeoms = (Eigen::DenseIndex)geom_model.ngeoms;
    PINOCCHIO_CHECK_ARGUMENT_SIZE(security_margin_map.rows(),ngeoms,"Input map does not have the correct number of rows.");
    PINOCCHIO_CHECK_ARGUMENT_SIZE(security_margin_map.cols(),ngeoms,"Input map does not have the correct number of columns.");
    PINOCCHIO_CHECK_ARGUMENT_SIZE(geom_model.collisionPairs.size(),collisionRequests.size(),"Current geometry data and the input geometry model are not conistent.");
    
    for(size_t k = 0; k < geom_model.collisionPairs.size(); ++k)
    {
      const CollisionPair & cp = geom_model.collisionPairs[k];
      
      Eigen::DenseIndex i,j;
      if(upper)
      {
        j = (Eigen::DenseIndex)std::max(cp.first,cp.second);
        i = (Eigen::DenseIndex)std::min(cp.first,cp.second);
      }
      else
      {
        i = (Eigen::DenseIndex)std::max(cp.first,cp.second);
        j = (Eigen::DenseIndex)std::min(cp.first,cp.second);
      }
      
      collisionRequests[k].security_margin = security_margin_map(i,j);
    }
  }

  inline void GeometryData::deactivateCollisionPair(const PairIndex pairId)
  {
    PINOCCHIO_CHECK_INPUT_ARGUMENT(pairId < activeCollisionPairs.size(),
                                   "The input argument pairId is larger than the number of collision pairs contained in activeCollisionPairs.");
    activeCollisionPairs[pairId] = false;
  }

  inline void GeometryData::deactivateAllCollisionPairs()
  {
    std::fill(activeCollisionPairs.begin(),activeCollisionPairs.end(),false);
  }

} // namespace pinocchio

/// @endcond

#endif // ifndef __pinocchio_multibody_geometry_hxx__
