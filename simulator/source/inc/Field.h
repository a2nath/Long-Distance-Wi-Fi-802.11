#pragma once
template<class T = double>
class Field
{
private:
	T value;
	uint size;
public:
	Field() : value(0), size(0) {}
	Field(T data, uint size) :
		value(data), size(size) {}
	void setval(T data) { this->value = data; }
	void setsize(float size) { this->size = size; }
	T data() { return this->value; }
	uint gsize() { return this->size; }
};