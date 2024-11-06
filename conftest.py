import pytest

import strai


@pytest.fixture(autouse=True, scope="module")
def shutdown_strai():
    strai.shutdown()
    yield


@pytest.fixture(autouse=True)
def preserve_block_order():
    strai.data.context.DataContext.get_current().execution_options.preserve_order = True
    yield


@pytest.fixture(autouse=True)
def disable_start_message():
    context = strai.data.context.DataContext.get_current()
    original_value = context.print_on_execution_start
    context.print_on_execution_start = False
    yield
    context.print_on_execution_start = original_value
