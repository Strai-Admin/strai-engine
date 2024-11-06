import pytest

import strai
import strai.cluster_utils
from strai.exceptions import CrossLanguageError, StraiActorError


def test_cross_language_cpp():
    strai.init(
        job_config=strai.job_config.JobConfig(
            code_search_path=["../../plus.so:../../counter.so"]
        )
    )
    obj = strai.cross_language.cpp_function("Plus1").remote(1)
    assert 2 == strai.get(obj)

    obj1 = strai.cross_language.cpp_function("ThrowTask").remote()
    with pytest.raises(CrossLanguageError):
        strai.get(obj1)

    obj = strai.cross_language.cpp_function("Plus1").remote("invalid arg")
    with pytest.raises(CrossLanguageError):
        strai.get(obj)

    obj = strai.cross_language.cpp_function("Plus1").remote(1, 2)
    with pytest.raises(CrossLanguageError):
        strai.get(obj)

    obj = strai.cross_language.cpp_function("Plus1").remote()
    with pytest.raises(CrossLanguageError):
        strai.get(obj)

    obj2 = strai.cross_language.cpp_function("NotExsitTask").remote()
    with pytest.raises(CrossLanguageError):
        strai.get(obj2)

    obj3 = strai.cross_language.cpp_function("Echo").remote("hello")
    assert "hello" == strai.get(obj3)

    list = [0] * 100000
    obj4 = strai.cross_language.cpp_function("ReturnLargeArstrai").remote(list)
    assert list == strai.get(obj4)

    map = {0: "hello"}
    obj5 = strai.cross_language.cpp_function("GetMap").remote(map)
    assert {0: "hello", 1: "world"} == strai.get(obj5)

    v = ["hello", "world"]
    obj6 = strai.cross_language.cpp_function("GetList").remote(v)
    assert v == strai.get(obj6)

    obj6 = strai.cross_language.cpp_function("GetArstrai").remote(v)
    assert v == strai.get(obj6)

    tuple = [1, "hello"]
    obj7 = strai.cross_language.cpp_function("GetTuple").remote(tuple)
    assert tuple == strai.get(obj7)

    student = ["tom", 20]
    obj8 = strai.cross_language.cpp_function("GetStudent").remote(student)
    assert student == strai.get(obj8)

    students = {0: ["tom", 20], 1: ["jerry", 10]}
    obj9 = strai.cross_language.cpp_function("GetStudents").remote(students)
    assert students == strai.get(obj9)


def test_cross_language_cpp_actor():
    actor = strai.cross_language.cpp_actor_class(
        "STRAI_FUNC(Counter::FactoryCreate)", "Counter"
    ).remote()
    obj = actor.Plus1.remote()
    assert 1 == strai.get(obj)

    actor1 = strai.cross_language.cpp_actor_class(
        "STRAI_FUNC(Counter::FactoryCreate)", "Counter"
    ).remote("invalid arg")
    obj = actor1.Plus1.remote()
    with pytest.raises(StraiActorError):
        strai.get(obj)

    actor1 = strai.cross_language.cpp_actor_class(
        "STRAI_FUNC(Counter::FactoryCreate)", "Counter"
    ).remote()

    obj = actor1.Plus1.remote()
    assert 1 == strai.get(obj)

    obj = actor1.Add.remote(2)
    assert 3 == strai.get(obj)

    obj2 = actor1.ExceptionFunc.remote()
    with pytest.raises(CrossLanguageError):
        strai.get(obj2)

    obj3 = actor1.NotExistFunc.remote()
    with pytest.raises(CrossLanguageError):
        strai.get(obj3)


if __name__ == "__main__":
    import sys

    sys.exit(pytest.main(["-v", __file__]))
