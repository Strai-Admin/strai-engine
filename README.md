

Strai is a unified framework for scaling AI and Python applications. Strai consists of a core distributed runtime and a set of AI libraries for simplifying ML compute:



Learn more about `Strai AI Libraries`_:

- Data : Scalable Datasets for ML
- Train : Distributed Training
- Tune : Scalable Hyperparameter Tuning
- RLlib : Scalable Reinforcement Learning
- Serve : Scalable and Programmable Serving

Or more about Strai Core and its key abstractions:

- Tasks : Stateless functions executed in the cluster.
- Actors : Stateful worker processes created in the cluster.
- Objects : Immutable values accessible across the cluster.

Learn more about Monitoring and Debugging:

- Monitor Strai apps and clusters with the Strai Dashboard .
- Debug Strai apps with the Strai Distributed Debugger .

Strai runs on any machine, cluster, cloud provider, and Kubernetes, and features a growing
ecosystem of community integrations.

Install Strai with: pip install strai. For nightly wheels, see the
Installation page.


Why Strai?
--------

Today's ML workloads are increasingly compute-intensive. As convenient as they are, single-node development environments such as your laptop cannot scale to meet these demands.

Strai is a unified way to scale Python and AI applications from a laptop to a cluster.

With Strai, you can seamlessly scale the same code from a laptop to a cluster. Strai is designed to be general-purpose, meaning that it can performantly run any kind of workload. If your application is written in Python, you can scale it with Strai, no other infrastructure required.


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

.. _`Discourse Forum`: https://t.me/strai_io
.. _`GitHub Issues`: https://t.me/strai_io_chat
.. _`StackOverflow`: https://t.me/strai_io_chat
.. _`Meetup Group`: https://t.me/strai_io_chat
.. _`Twitter`: https://x.com/strai_io
.. _`Slack`: https://t.me/strai_io_chat
