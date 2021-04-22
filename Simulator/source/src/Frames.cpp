#include "../inc/Frames.h"

/*
* Generic frame functions
*/
//
Frame::Frame(uint time, uint source, uint seqNum, uint dest, uint mcs, uint datsize, uint fragment) :
	departureTime(time), control(Field<uint>(0, 2)), payload_bytes(Field<uint>(0, 2)), trans_address(Field<uint>(INFINITY, 6)),
	rec_address(Field<uint>(INFINITY, 6)), source_address(Field<uint>(source, 6)), fragmentNum(Field<uint>(fragment, 0)),
	sequenceNum(Field<uint>(seqNum, 2)), destin_address(Field<uint>(dest, 6)), trailer(Field<uint>(INFINITY, 4)), modulation_sc(mcs),
	data_ppdu_size(datsize)
{
	mpdu_header_size = control.gsize() + trans_address.gsize() + rec_address.gsize() + source_address.gsize() + sequenceNum.gsize()
		+ trailer.gsize() + payload_bytes.gsize() + destin_address.gsize(); //bytes
}
/*
* RTS frame functions
*/
RTS::RTS(uint time, uint source, uint seqNum, uint dest, uint mcs, uint datsize, uint fragment) :
	Frame(time, source, seqNum, dest, mcs, datsize, fragment)
{
	subtype_value = Global::_RTS;
	duration_us = data_size(0 + mpdu_header_size, modulation_sc);
}
void RTS::change_time(uint time)
{
	this->departureTime = time;
}
uint RTS::getFrag()
{
	return fragmentNum.data();
}
uint RTS::getTime()
{
	return departureTime;
}
uint RTS::getSource()
{
	return source_address.data();
}
uint RTS::getDest()
{
	return destin_address.data();
}
uint RTS::getDuration()
{
	return duration_us;
}
uint RTS::subval()
{
	return subtype_value;
}
bool RTS::subval(int query_type)
{
	return subtype_value == query_type;
}
uint RTS::getSequence()
{
	return this->sequenceNum.data();
}
void RTS::setCtrl(uint flag)
{
	control.setval(control.data() | flag);
}
void RTS::unsetCtrl(uint flag)
{
	control.setval(control.data() & ~flag);
}
uint RTS::getCtrl()
{
	return control.data();
}
uint RTS::get_dat_duration()
{
	return data_ppdu_size;
}
uint RTS::get_mcs_idx()
{
	return modulation_sc;
}
uint RTS::get_header_size()
{
	return mpdu_header_size;
}
/*
* CTS frame functions
*/
CTS::CTS(uint time, uint source, uint seqNum, uint dest, uint mcs, uint datsize, uint fragment) :
	Frame(time, source, seqNum, dest, mcs, datsize, fragment)
{
	subtype_value = Global::_CTS;
	duration_us = data_size(0 + mpdu_header_size, modulation_sc);
}
void CTS::change_time(uint time)
{
	this->departureTime = time;
}
uint CTS::getFrag()
{
	return fragmentNum.data();
}
uint CTS::getTime()
{
	return departureTime;
}
uint CTS::getSource()
{
	return source_address.data();
}
uint CTS::getDest()
{
	return destin_address.data();
}
uint CTS::getDuration()
{
	return duration_us;
}
uint CTS::subval()
{
	return subtype_value;
}
bool CTS::subval(int query_type)
{
	return subtype_value == query_type;
}
uint CTS::getSequence()
{
	return this->sequenceNum.data();
}
void CTS::setCtrl(uint flag)
{
	control.setval(control.data() | flag);
}
void CTS::unsetCtrl(uint flag)
{
	control.setval(control.data() & ~flag);
}
uint CTS::getCtrl()
{
	return control.data();
}
uint CTS::get_dat_duration()
{
	return data_ppdu_size;
}
uint CTS::get_mcs_idx()
{
	return modulation_sc;
}
uint CTS::get_header_size()
{
	return mpdu_header_size;
}
/*
* DATA frame functions
*/
DATA::DATA(uint time, uint source, uint seqNum, uint dest, uint mcs, uint datsize, uint fragment) :
	Frame(time, source, seqNum, dest, mcs, datsize, fragment)
{
	subtype_value = Global::_DATA;
	duration_us = datsize;
}
void DATA::change_time(uint time)
{
	this->departureTime = time;
}
uint DATA::getFrag()
{
	return fragmentNum.data();
}
uint DATA::getTime()
{
	return departureTime;
}
uint DATA::getSource()
{
	return source_address.data();
}
uint DATA::getDest()
{
	return destin_address.data();
}
uint DATA::getDuration()
{
	return duration_us;
}
uint DATA::subval()
{
	return subtype_value;
}
bool DATA::subval(int query_type)
{
	return subtype_value == query_type;
}
uint DATA::getSequence()
{
	return this->sequenceNum.data();
}
void DATA::setCtrl(uint flag)
{
	control.setval(control.data() | flag);
}
void DATA::unsetCtrl(uint flag)
{
	control.setval(control.data() & ~flag);
}
uint DATA::getCtrl()
{
	return control.data();
}
uint DATA::get_dat_duration()
{
	return data_ppdu_size;
}
uint DATA::get_mcs_idx()
{
	return modulation_sc;
}
uint DATA::get_header_size()
{
	return mpdu_header_size;
}
/*
* ACK frame functions
*/
ACK::ACK(uint time, uint source, uint seqNum, uint dest, uint mcs, uint datsize, uint fragment) :
	Frame(time, source, seqNum, dest, mcs, datsize, fragment)
{
	subtype_value = Global::_ACK;;
	duration_us = data_size(0 + mpdu_header_size, modulation_sc); //us
}
void ACK::change_time(uint time)
{
	this->departureTime = time;
}
uint ACK::getFrag()
{
	return fragmentNum.data();
}
uint ACK::getTime()
{
	return departureTime;
}
uint ACK::getSource()
{
	return source_address.data();
}
uint ACK::getDest()
{
	return destin_address.data();
}
uint ACK::getDuration()
{
	return duration_us;
}
uint ACK::subval()
{
	return subtype_value;
}
bool ACK::subval(int query_type)
{
	return subtype_value == query_type;
}
uint ACK::getSequence()
{
	return this->sequenceNum.data();
}
void ACK::setCtrl(uint flag)
{
	control.setval(control.data() | flag);
}
void ACK::unsetCtrl(uint flag)
{
	control.setval(control.data() & ~flag);
}
uint ACK::getCtrl()
{
	return control.data();
}
uint ACK::get_dat_duration()
{
	return data_ppdu_size;
}
uint ACK::get_mcs_idx()
{
	return modulation_sc;
}
uint ACK::get_header_size()
{
	return mpdu_header_size;
}