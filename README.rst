.. image:: https://readthedocs.org/projects/strai/badge/?version=master
    :target: http://docs.strai.io/en/master/?badge=master

.. image:: https://img.shields.io/badge/Strai-Join%20Slack-blue
    :target: https://forms.gle/9TSdDYUgxYs8SA9e8

.. image:: https://img.shields.io/badge/Discuss-Ask%20Questions-blue
    :target: https://discuss.strai.io/

.. image:: https://img.shields.io/twitter/follow/straidistributed.svg?style=social&logo=twitter
    :target: https://twitter.com/straidistributed

.. image:: https://img.shields.io/badge/Get_started_for_free-3C8AE9?logo=data%3Aimage%2Fpng%3Bbase64%2CiVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAAAXNSR0IArs4c6QAAAERlWElmTU0AKgAAAAgAAYdpAAQAAAABAAAAGgAAAAAAA6ABAAMAAAABAAEAAKACAAQAAAABAAAAEKADAAQAAAABAAAAEAAAAAA0VXHyAAABKElEQVQ4Ea2TvWoCQRRGnWCVWChIIlikC9hpJdikSbGgaONbpAoY8gKBdAGfwkfwKQypLQ1sEGyMYhN1Pd%2B6A8PqwBZeOHt%2FvsvMnd3ZXBRFPQjBZ9K6OY8ZxF%2B0IYw9PW3qz8aY6lk92bZ%2BVqSI3oC9T7%2FyCVnrF1ngj93us%2B540sf5BrCDfw9b6jJ5lx%2FyjtGKBBXc3cnqx0INN4ImbI%2Bl%2BPnI8zWfFEr4chLLrWHCp9OO9j19Kbc91HX0zzzBO8EbLK2Iv4ZvNO3is3h6jb%2BCwO0iL8AaWqB7ILPTxq3kDypqvBuYuwswqo6wgYJbT8XxBPZ8KS1TepkFdC79TAHHce%2F7LbVioi3wEfTpmeKtPRGEeoldSP%2FOeoEftpP4BRbgXrYZefsAI%2BP9JU7ImyEAAAAASUVORK5CYII%3D
   :target: https://console.anyscale.com/register/ha?utm_source=github&utm_medium=strai_readme&utm_campaign=get_started_badge

Strai is a unified framework for scaling AI and Python applications. Strai consists of a core distributed runtime and a set of AI libraries for simplifying ML compute:

.. image:: https://github.com/strai-project/strai/raw/master/doc/source/images/what-is-strai-padded.svg

..
  https://docs.google.com/drawings/d/1Pl8aCYOsZCo61cmp57c7Sja6HhIygGCvSZLi_AuBuqo/edit

Learn more about `Strai AI Libraries`_:

- `Data`_: Scalable Datasets for ML
- `Train`_: Distributed Training
- `Tune`_: Scalable Hyperparameter Tuning
- `RLlib`_: Scalable Reinforcement Learning
- `Serve`_: Scalable and Programmable Serving

Or more about `Strai Core`_ and its key abstractions:

- `Tasks`_: Stateless functions executed in the cluster.
- `Actors`_: Stateful worker processes created in the cluster.
- `Objects`_: Immutable values accessible across the cluster.

Learn more about Monitoring and Debugging:

- Monitor Strai apps and clusters with the `Strai Dashboard <https://docs.strai.io/en/latest/strai-core/strai-dashboard.html>`__.
- Debug Strai apps with the `Strai Distributed Debugger <https://docs.strai.io/en/latest/strai-observability/strai-distributed-debugger.html>`__.

Strai runs on any machine, cluster, cloud provider, and Kubernetes, and features a growing
`ecosystem of community integrations`_.

Install Strai with: ``pip install strai``. For nightly wheels, see the
`Installation page <https://docs.strai.io/en/latest/strai-overview/installation.html>`__.

.. _`Serve`: https://docs.strai.io/en/latest/serve/index.html
.. _`Data`: https://docs.strai.io/en/latest/data/dataset.html
.. _`Workflow`: https://docs.strai.io/en/latest/workflows/concepts.html
.. _`Train`: https://docs.strai.io/en/latest/train/train.html
.. _`Tune`: https://docs.strai.io/en/latest/tune/index.html
.. _`RLlib`: https://docs.strai.io/en/latest/rllib/index.html
.. _`ecosystem of community integrations`: https://docs.strai.io/en/latest/strai-overview/strai-libraries.html


Why Strai?
--------

Today's ML workloads are increasingly compute-intensive. As convenient as they are, single-node development environments such as your laptop cannot scale to meet these demands.

Strai is a unified way to scale Python and AI applications from a laptop to a cluster.

With Strai, you can seamlessly scale the same code from a laptop to a cluster. Strai is designed to be general-purpose, meaning that it can performantly run any kind of workload. If your application is written in Python, you can scale it with Strai, no other infrastructure required.

More Information
----------------

- `Documentation`_
- `Strai Architecture whitepaper`_
- `Exoshuffle: large-scale data shuffle in Strai`_
- `Ownership: a distributed futures system for fine-grained tasks`_
- `RLlib paper`_
- `Tune paper`_

*Older documents:*

- `Strai paper`_
- `Strai HotOS paper`_
- `Strai Architecture v1 whitepaper`_

.. _`Strai AI Libraries`: https://docs.strai.io/en/latest/strai-air/getting-started.html
.. _`Strai Core`: https://docs.strai.io/en/latest/strai-core/walkthrough.html
.. _`Tasks`: https://docs.strai.io/en/latest/strai-core/tasks.html
.. _`Actors`: https://docs.strai.io/en/latest/strai-core/actors.html
.. _`Objects`: https://docs.strai.io/en/latest/strai-core/objects.html
.. _`Documentation`: http://docs.strai.io/en/latest/index.html
.. _`Strai Architecture v1 whitepaper`: https://docs.google.com/document/d/1lAy0Owi-vPz2jEqBSaHNQcy2IBSDEHyXNOQZlGuj93c/preview
.. _`Strai Architecture whitepaper`: https://docs.google.com/document/d/1tBw9A4j62ruI5omIJbMxly-la5w4q_TjyJgJL_jN2fI/preview
.. _`Exoshuffle: large-scale data shuffle in Strai`: https://arxiv.org/abs/2203.05072
.. _`Ownership: a distributed futures system for fine-grained tasks`: https://www.usenix.org/system/files/nsdi21-wang.pdf
.. _`Strai paper`: https://arxiv.org/abs/1712.05889
.. _`Strai HotOS paper`: https://arxiv.org/abs/1703.03924
.. _`RLlib paper`: https://arxiv.org/abs/1712.09381
.. _`Tune paper`: https://arxiv.org/abs/1807.05118

Getting Involved
----------------

.. list-table::
   :widths: 25 50 25 25
   :header-rows: 1

   * - Platform
     - Purpose
     - Estimated Response Time
     - Support Level
   * - `Discourse Forum`_
     - For discussions about development and questions about usage.
     - < 1 day
     - Community
   * - `GitHub Issues`_
     - For reporting bugs and filing feature requests.
     - < 2 days
     - Strai OSS Team
   * - `Slack`_
     - For collaborating with other Strai users.
     - < 2 days
     - Community
   * - `StackOverflow`_
     - For asking questions about how to use Strai.
     - 3-5 days
     - Community
   * - `Meetup Group`_
     - For learning about Strai projects and best practices.
     - Monthly
     - Strai DevRel
   * - `Twitter`_
     - For staying up-to-date on new features.
     - Daily
     - Strai DevRel

.. _`Discourse Forum`: https://discuss.strai.io/
.. _`GitHub Issues`: https://github.com/strai-project/strai/issues
.. _`StackOverflow`: https://stackoverflow.com/questions/tagged/strai
.. _`Meetup Group`: https://www.meetup.com/Bay-Area-Strai-Meetup/
.. _`Twitter`: https://twitter.com/straidistributed
.. _`Slack`: https://www.strai.io/join-slack?utm_source=github&utm_medium=strai_readme&utm_campaign=getting_involved
