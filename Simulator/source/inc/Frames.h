#pragma once
#include <string>
#include "common.h"
#include "Field.h"
using namespace std;

class Frame
{
protected:
	Field<uint> control, trans_address, rec_address, fragmentNum, sequenceNum, trailer,
		payload_bytes, source_address, destin_address;
	uint duration_us, departureTime, mpdu_header_size, modulation_sc, data_ppdu_size;
	Global::frame_map subtype_value;

public:
	Frame() {}
	virtual ~Frame() {}
	Frame(uint time, uint source, uint seqNum, uint dest, uint mcs, uint datsize, uint fragment = 1);
	virtual void change_time(uint new_time) = 0;
	virtual uint getTime() = 0;
	virtual uint getSource() = 0;
	virtual uint getDest() = 0;
	virtual uint getDuration() = 0;
	virtual uint subval() = 0;
	virtual bool subval(int query) = 0;
	virtual uint getSequence() = 0;
	virtual void setCtrl(uint flag) = 0;
	virtual void unsetCtrl(uint flag) = 0;
	virtual uint getCtrl() = 0;
	virtual uint get_dat_duration() = 0;
	virtual uint get_mcs_idx() = 0;
	virtual uint get_header_size() = 0;
	virtual uint getFrag() = 0;
};

class RTS : public Frame
{
private:
public:
	RTS() {}
	~RTS() {}
	RTS(uint time, uint source, uint seqNum, uint dest, uint mcs, uint datsize, uint fragment = 1);
	virtual void change_time(uint time) override;
	virtual uint getTime() override;
	virtual uint getSource() override;
	virtual uint getDest() override;
	virtual uint getDuration() override;
	virtual uint subval() override;
	virtual bool subval(int query_type) override;
	virtual uint getSequence() override;
	void setCtrl(uint flag);
	void unsetCtrl(uint flag);
	uint getCtrl();
	uint get_dat_duration();
	uint get_mcs_idx();
	uint get_header_size();
	uint getFrag();
};

class CTS : public Frame
{
private:
public:
	CTS() {}
	~CTS() {}
	CTS(uint time, uint source, uint seqNum, uint dest, uint mcs, uint datsize, uint fragment = 1);
	virtual void change_time(uint time) override;
	virtual uint getTime() override;
	virtual uint getSource() override;
	virtual uint getDest() override;
	virtual uint getDuration() override;
	virtual uint subval() override;
	virtual bool subval(int query_type) override;
	virtual uint getSequence() override;
	void setCtrl(uint flag);
	void unsetCtrl(uint flag);
	uint getCtrl();
	uint get_dat_duration();
	uint get_mcs_idx();
	uint get_header_size();
	uint getFrag();
};

class DATA : public Frame
{
private:
public:
	DATA() {}
	~DATA() {}
	DATA(uint time, uint source, uint seqNum, uint dest, uint mcs, uint datsize, uint fragment = 1);
	virtual void change_time(uint time) override;
	virtual uint getTime() override;
	virtual uint getSource() override;
	virtual uint getDest() override;
	virtual uint getDuration() override;
	virtual uint subval() override;
	virtual bool subval(int query_type) override;
	virtual uint getSequence() override;
	void setCtrl(uint flag);
	void unsetCtrl(uint flag);
	uint getCtrl();
	uint get_dat_duration();
	uint get_mcs_idx();
	uint get_header_size();
	uint getFrag();
};

class ACK : public Frame
{
private:
public:
	ACK() {}
	~ACK() {}
	ACK(uint time, uint source, uint seqNum, uint dest, uint mcs, uint datsize, uint fragment = 1);
	virtual void change_time(uint time) override;
	virtual uint getTime() override;
	virtual uint getSource() override;
	virtual uint getDest() override;
	virtual uint getDuration() override;
	virtual uint subval() override;
	virtual bool subval(int query_type) override;
	virtual uint getSequence() override;
	void setCtrl(uint flag);
	void unsetCtrl(uint flag);
	uint getCtrl();
	uint get_dat_duration();
	uint get_mcs_idx();
	uint get_header_size();
	uint getFrag();
};
typedef std::shared_ptr<Frame> sptrFrame;