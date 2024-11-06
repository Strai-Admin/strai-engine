/// This is an example of Strai C++ application. Please visit
/// `https://docs.strai.io/en/master/strai-core/walkthrough.html#installation`
/// for more details.

/// including the `<strai/api.h>` header
#include <strai/api.h>

/// common function
int Plus(int x, int y) { return x + y; }
/// Declare remote function
STRAI_REMOTE(Plus);

/// class
class Counter {
 public:
  int count;

  Counter(int init) { count = init; }
  /// static factory method
  static Counter *FactoryCreate(int init) { return new Counter(init); }

  /// non static function
  int Add(int x) {
    count += x;
    return count;
  }
};
/// Declare remote function
STRAI_REMOTE(Counter::FactoryCreate, &Counter::Add);

int main(int argc, char **argv) {
  /// initialization
  strai::Init();

  /// put and get object
  auto object = strai::Put(100);
  auto put_get_result = *(strai::Get(object));
  std::cout << "put_get_result = " << put_get_result << std::endl;

  /// common task
  auto task_object = strai::Task(Plus).Remote(1, 2);
  int task_result = *(strai::Get(task_object));
  std::cout << "task_result = " << task_result << std::endl;

  /// actor
  strai::ActorHandle<Counter> actor = strai::Actor(Counter::FactoryCreate).Remote(0);
  /// actor task
  auto actor_object = actor.Task(&Counter::Add).Remote(3);
  int actor_task_result = *(strai::Get(actor_object));
  std::cout << "actor_task_result = " << actor_task_result << std::endl;
  /// actor task with reference argument
  auto actor_object2 = actor.Task(&Counter::Add).Remote(task_object);
  int actor_task_result2 = *(strai::Get(actor_object2));
  std::cout << "actor_task_result2 = " << actor_task_result2 << std::endl;

  /// shutdown
  strai::Shutdown();
  return 0;
}
