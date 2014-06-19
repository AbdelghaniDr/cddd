#ifndef CDDD_CQRS_AGGREATE_H__
#define CDDD_CQRS_AGGREATE_H__

#include "cddd/cqrs/artifact.h"


namespace cddd {
namespace cqrs {

class aggregate : public artifact {
public:
   aggregate() = default;
   aggregate(const aggregate &) = delete;
   aggregate(aggregate &&) = default;

   virtual ~aggregate() = default;

   aggregate & operator =(const aggregate &) = delete;
   aggregate & operator =(aggregate &&) = default;

   inline const object_id & id() const {
      return aggregate_id;
   }

protected:
   inline aggregate(object_id id_, std::shared_ptr<event_dispatcher> dispatcher_) :
      artifact(dispatcher_),
      aggregate_id(id_)
   {
   }

private:
   object_id aggregate_id;
};

}
}

#endif
