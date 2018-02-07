#### Secure content-based routing (SCBR)

Content-based routing (CBR) is a flexible and powerful paradigm for scalable communication among distributed processes.
It decouples data producers from consumers, and dynamically routes messages based on their content.
CBR requires router components to filter messages by matching their content against a (potentially large) collection of subscriptions that act as a reverse index and, hence, must be stored by the filtering engines.
In turn, this requires the router to see the content of both the messages and the subscriptions, which represents a major privacy threat.
Our system, called SCBR, is a state-of-the-art routing engine to provide both security and performance while executing under the protection of a secure SGX enclave.

In such a system, we have three roles:
* Filter : matches publications with subscriptions and forwards data to intended destinations
* Consumer (or subscriber) : consumes data
* Producer (or publisher) : produces data
Consumer and producer roles can coexist in a single process.

The Filter engine itself is autonomous.
The system interface lies in the implementation of consumers and producers by following the communication protocols and mesage format.

SCBR is oblivious to criptographic mechanisms for the payload of publications.
Therefore, they should be implemented by applications that use the SCBR infrastructure.
Encryption of headers, however, is currently made through a shared key hardcoded in the source code of producers and the Filter engine.
This approach is not safe and it should not be deployed as it is.
The solution for such issue is under development.


[More info](https://arxiv.org/abs/1701.04612)

**Source code:**
```
$ git clone git@gitlab.securecloud.works:rafael.pires/sgx_common.git
$ git clone git@gitlab.securecloud.works:rafael.pires/scbr.git

Or, alternatively:
$ git clone https://gitlab.securecloud.works/rafael.pires/sgx_common.git
$ git clone https://gitlab.securecloud.works/rafael.pires/scbr.git
```

**To compile:**
```
$ cd scbr && make
```

**To test:**
```
$ make test
```

##### Message interface
Publication and subscription headers are in ASCII format, as follows:

Publications header contain a set of pairs of key and numeric values.
The payload of such messages is arbitrary, defined by the application that runs on top of it.
```
0 0 P i symbol 58 i date 1136764800 i high 0 i low 0 i volume 2250
```

Subscription headers, besides key and values, also include a comparison operator
(=, >, <, >=, <=).
Each field is separated by the letter i with spaces before and after.
The second field (8945 in the sample below) corresponds to the subscriber identifier.
This is how the filter will refer to matching subscribers, so that the underlying communication channel can forward the payloads to the intended destinations.
```
0 8945 S i symbol = 29 i open >= 132 i open <= 427
```

__Input:__ Header and Payload

__Output:__ Serialized message that can be sent through the communication channel

__Users:__ Producers and Consumers

##### Filter interface

__Input:__ The filter receives two Base64 encoded strings separated by a space. The first corresponds to the header while the second to an arbitrary and opaque payload (that can be void - usually the case for subscriptions).

__Output:__ If the message is a subscription, it safely stores it. If it is a publication that matches to stored subscriptions, its payload is forwarded to them.

__Users:__ data providers, applications, other services

##### Communication interface
To comunicate with the Filter interface, one must use ZeroMQ message library.
We provide a communication helper class to do so.

__Input:__
* ZeroMQ context
* Bind or connection address
* Identification string

__Output:__ communication object

__Users:__ producers and consumers


