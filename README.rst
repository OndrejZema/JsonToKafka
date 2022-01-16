JsonKafka (output plugin)
=========================

Plugin for converting messages from IPFIX to JSON and saving to apache kafka. The 
conversion process is multithread with an adjustableinput buffer size. The number of thread is 
automatically set accoring to the PC.
This plugin needs to compiled with c++ 17 support.

Example configuration
=====================

.. code-block:: xml
<output>
	<name>JsonToKafka - plugin</name>
	<plugin>JsonToKafka</plugin>
	<params>
		<tcpFlags>formatted</tcpFlags>
		<timestamp>formatted</timestamp>
		<protocol>formatted</protocol>
		<ignoreUnknown>false</ignoreUnknown>
		<ignoreOptions>false</ignoreOptions>
		<nonPrintableChar>true</nonPrintableChar>
		<numericNames>false</numericNames>
		<octetArrayAsUint>false</octetArrayAsUint>
		<splitBiflow>false</splitBiflow>
		<kafka>
			<hostName>localhost</hostName>
			<port>9092</port>
			<group></group>
			<topicList>testTopic</topicList>
		</kafka>
		<parser>
			<processMessageLength>1024</processMessageLength>
			<messagesBufferSize>512</messagesBufferSize>
		</parser>
	</params>
</output>

Parameters
==========

Formatting parameters:

:``tcpFlags``:
    Convert TCP flags to common textual representation (formatted, e.g. ".A..S.")
    or to a number (raw). [values: formatted/raw, default: formatted]

:``timestamp``:
    Convert timestamp to ISO 8601 textual representation (all timestamps in UTC and milliseconds,
    e.g. "2018-01-22T09:29:57.828Z") or to a unix timestamp (all timestamps in milliseconds).
    [values: formatted/unix, default: formatted]

:``protocol``:
    Convert protocol identification to formatted style (e.g. instead 6 writes "TCP") or to a number.
    [values: formatted/raw, default: formatted]

:``ignoreUnknown``:
    Skip unknown Information Elements (i.e. record fields with unknown name and data type).
    If disabled, data of unknown elements are formatted as unsigned integer or hexadecimal values.
    For more information, see ``octetArrayAsUint`` option. [values: true/false, default: true]

:``ignoreOptions``:
    Skip non-flow records used for reporting metadata about IPFIX Exporting and Metering Processes
    (i.e. records described by Options Templates). [values: true/false, default: true]

:``nonPrintableChar``:
    Ignore non-printable characters (newline, tab, control characters, etc.) in IPFIX strings.
    If disabled, these characters are escaped on output. [values: true/false, default: true]

:``octetArrayAsUint``:
    Converter each IPFIX field with octetArray type (including IPFIX fields with unknown
    definitions) as unsigned integer if the size of the field is less or equal to 8 bytes.
    Fields with the size above the limit are always converted as string representing hexadecimal
    value, which is typically in network byte order (e.g. "0x71E1"). Keep on mind, that there might
    be octetArray fields with variable length that might be interpreted differently based on their
    size. If disabled, octetArray fields are never interpreted as unsigned integers.
    [values: true/false, default: true]

:``numericNames``:
    Use only short identification of Information Elements (i.e. "enXX:idYY"). If enabled, the
    short version is used even if the definition of the field is known. This option can help to
    create a shorter JSON records with key identifiers which are independent on the internal
    configuration. [values: true/false, default: false]

:``splitBiflow``:
    In case of Biflow records, split the record to two unidirectional flow records. Non-biflow
    records are unaffected. [values: true/false, default: false]

---

Kafka parameters:

:``hostName``:
	Apache kafka service address [values: text, default: 127.0.0.1]
:``port``:
	Apache kafka service port [values: number, default: 9092]
:``group``:
	[values: text, default:]
:``topicList``:
	Message topic for apache kafka [values: text, default: ---]

---

Parser parameters:

:``processMessageLength``:
	Message length to convert (during the process the size is dynamically increased as need) [values: number, default: 1024]
:``messagesBufferSize``:
	The size of the masseges input buffer [values: number, default: 1024]

