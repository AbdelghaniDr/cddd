#ifndef CDDD_CQRS_ARTIFACT_STORE_H__
#define CDDD_CQRS_ARTIFACT_STORE_H__

#include "cqrs/event.h"
#include "cqrs/exceptions.h"
#include "cqrs/store.h"
#include "cqrs/traits.h"


namespace cddd {
namespace cqrs {

template<class ArtifactType, class EventSource, class StreamFactory, class ObjectFactory>
class artifact_store : public store<ArtifactType> {
public:
   static_assert(is_store<EventSource>::value &&
                 std::is_same<typename EventSource::value_type, event>::value &&
                 has_const_load<typename EventSource::stream_type, event_sequence(std::size_t, std::size_t)>::value,
                 "EventSource must have store interface.");
   typedef ArtifactType value_type;
   typedef typename store<ArtifactType>::pointer pointer;
   typedef EventSource event_source_type;
   typedef typename EventSource::stream_type event_stream_type;
   typedef StreamFactory stream_factory;
   typedef ObjectFactory object_factory;

   explicit inline artifact_store(std::unique_ptr<event_source_type> es, stream_factory sf, object_factory of) :
      events_provider(std::move(es)),
      create_stream(std::forward<stream_factory>(sf)),
      create_object(std::forward<object_factory>(of)) {
   }

   artifact_store(const artifact_store &) = delete;
   artifact_store(artifact_store &&) = default;

   virtual ~artifact_store() = default;

   artifact_store &operator =(const artifact_store &) = delete;
   artifact_store &operator =(artifact_store &&) = default;

   virtual bool has(object_id id) const final override {
      return !id.is_null() && events_provider->has(id);
   }

   virtual void put(pointer object) final override {
      validate_object(object);

      if (object->has_uncommitted_events()) {
         save_object(*object);
      }
   }

   virtual pointer get(object_id id) const final override {
      validate_object_id(id);
      return load_object(id, std::numeric_limits<std::size_t>::max());
   }

   virtual pointer get(object_id id, std::size_t version) const {
      validate_object_id(id);
      return load_object(id, version);
   }

private:
   static inline void validate_object(pointer object) {
      if (object == nullptr) {
         throw null_pointer_exception("object");
      }
      else if (object->id().is_null()) {
         throw null_id_exception("object->id()");
      }
   }

   static inline void validate_object_id(object_id id) {
      if (id.is_null()) {
         throw null_id_exception("id");
      }
   }

   inline void save_object(ArtifactType &object) {
      auto str = get_event_stream(object.id());
      str->save(object.uncommitted_events());
      events_provider->put(str);
      object.clear_uncommitted_events();
   }

   inline std::shared_ptr<event_stream_type> get_event_stream(object_id id) {
      return has(id) ? events_provider->get(id) : create_stream(id);
   }

   inline event_sequence load_events(object_id id, std::size_t min_revision, std::size_t max_revision) const {
      return events_provider->get(id)->load(min_revision, max_revision);
   }

   inline pointer load_object(object_id id, std::size_t revision) const {
      pointer object = create_object(id, revision);
      if (object->revision() < revision) {
         object->load_from_history(load_events(id, object->revision() + 1, revision));
      }
      return object;
   }

   std::unique_ptr<event_source_type> events_provider;
   stream_factory create_stream;
   object_factory create_object;
};

}
}

#endif
